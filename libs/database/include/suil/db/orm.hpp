//
// Created by Mpho Mbotho on 2020-11-19.
//

#ifndef SUILDB_ORM_HPP
#define SUILDB_ORM_HPP

#include <suil/db/symbols.scc.hpp>

#include <suil/base/sio.hpp>
#include <suil/base/string.hpp>
#include <suil/base/buffer.hpp>
#include <suil/base/logging.hpp>


namespace suil::db {
    template<typename T>
    using removeAutoIncrementFields = decltype(removeMembersWithAttr(std::declval<T>(), sym(AUTO_INCREMENT)));

    template <typename T>
    using removeIgnoreFields = decltype(removeMembersWithAttr(std::declval<T>(), sym(IGNORE)));

    template <typename T>
    using removeReadOnlyFields = decltype(removeMembersWithAttr(std::declval<T>(), sym(READ_ONLY)));

    template <typename T>
    using extractPrimaryKeyFields = decltype(extractMembersWithAttr(std::declval<T>(), sym(PRIMARY_KEY)));

    template <typename T>
    using extractUniqueFields = decltype(extractMembersWithAttr(std::declval<T>(), sym(UNIQUE)));

    template <typename T>
    using extractAutoIncrementFields = decltype(extractMembersWithAttr(std::declval<T>(), sym(AUTO_INCREMENT)));

    template <typename T>
    using removePrimaryKeyFields = decltype(removeMembersWithAttr(std::declval<T>(), sym(PRIMARY_KEY)));

    template <typename Connection, typename Type>
        requires iod::IsMetaType<Type>
    class Orm {
    public:
        using object_t = Type;
    protected:
        using Schema = typename Type::Schema;
        using WithoutAutoIncrement = removeAutoIncrementFields<Schema>;
        using PrimaryKeys = extractPrimaryKeyFields<Schema>;
        using WithoutIgnore = removeIgnoreFields<Schema>;
        using UniqueKeys = extractUniqueFields<Schema>;
        using AutoIncrementKeys = extractAutoIncrementFields<Schema>;

        template <typename Column>
        using IsUniqueKey = iod::has_symbol<UniqueKeys, typename Column::left_t>;
        template <typename Column>
        using IsPrimaryKey = iod::has_symbol<UniqueKeys, typename Column::left_t>;
        template <typename Column>
        using IsAutoIncrementKey = iod::has_symbol<AutoIncrementKeys, typename Column::left_t>;
        template <typename T>
        static constexpr bool IsQueryColumn = IsUniqueKey<T>::value or IsPrimaryKey<T>::value or IsAutoIncrementKey<T>::value;

        static_assert(!std::is_same_v<PrimaryKeys, void>, "ORM requires at least one member to be a primary key");

    public:
        Orm(suil::String table, Connection& conn)
            : mTable{std::move(table)},
              conn{conn.get()}
        {}

        template<typename Column, typename T>
            requires (IsQueryColumn<Column> and iod::IsMetaType<T>)
        bool find(const Column& column, T& o)
        {
            Buffer qb(32);
            qb << "SELECT ";
            bool first{true};
            iod::foreach(WithoutIgnore()) | [&](const auto& m) {
                if (!first) {
                    qb << ", ";
                }
                qb << m.symbol().name();
                first = false;
            };

            qb << " FROM " << mTable << " WHERE " << column.left.name() << " = ";
            Connection::params(qb, 1);

            return conn(qb)(column.right) >> o;
        }

        template<typename Column>
            requires IsQueryColumn<Column>
        bool has(Column column)
        {
            Buffer qb(32);
            qb << "SELECT COUNT(" << column.left.name() << ") FROM " << mTable << " WHERE " << column.left.name() << " = ";
            Connection::params(qb, 1);
            int count{0};
            if (conn(qb)(column.right) >> count) {
                return count != 0;
            }
            return false;
        }

        template<typename Value, typename Where>
            requires IsQueryColumn<Where>
        bool selectColumn(const String& col, Value& v, Where where)
        {
            Buffer qb(32);
            qb << "SELECT " << col << " FROM " << mTable << " WHERE " << where.left.name() << " = ";
            Connection::params(qb, 1);
            return conn(qb)(where.right) >> v;
        }

        int truncate()
        {
            Buffer qb(32);
            qb << "TRUNCATE TABLE " << mTable;
            int count{0};
            conn(qb)() >> count;
            return count;
        }

        template <typename T>
            requires iod::IsMetaType<T>
        bool insert(const T& o)
        {
            Buffer qb(64), vb(32);

            qb << "INSERT INTO " << mTable << " (";
            bool first{true};
            int  index{1};
            using _Temp = removeIgnoreFields<WithoutAutoIncrement>;
            auto values = iod::foreach2(_Temp{}) | [&](const auto& m) {
                if (!first) {
                    qb << ", ";
                    vb << ", ";
                }
                first = false;
                qb << m.symbol().name();
                Connection::params(vb, index++);

                return m.symbol() = m.symbol().member_access(o);
            };

            qb << ") VALUES (" << vb << ")";
            auto req = conn(qb);
            iod::apply(values, req);

            return req.status();
        }

        bool cifne(bool trunc = false)
        {
            if (conn.hasTable(mTable)) {
                return conn.createTable(mTable, std::declval<WithoutIgnore>());
            }

            if (trunc) {
                truncate();
                return true;
            }
            return false;
        }

        bool cifne(std::vector<Type>& seed, bool trunc = false)
        {
            // create table if does not exist
            if (Ego.cifne(trunc)) {
                // if created seed with data
                if (!seed.empty()) {
                    for(auto& data: seed) {
                        if(!Ego.insert(data)) {
                            // inserting seed user failed
                            ldebug(&conn, "inserting seed entry into table '%' failed", mTable());
                            return false;
                        }
                    }
                }
                return true;
            }
            return false;
        }

        template <typename Func>
        void foreach(Func func) {
            Buffer qb(32);
            qb << "SELECT * FROM " << mTable;
            conn(qb)() | func;
        }

        std::vector<Type> getAll() {
            std::vector<Type> data;
            Ego.foreach([&](Type&& o) {
                data.push_back(std::move(o));
            });

            return data;
        }

        template<typename T>
            requires iod::IsMetaType<T>
        bool update(const T& o)
        {
            auto pk = iod::intersect(typename T::Schema(), PrimaryKeys());
            static_assert(decltype(pk)::size() > 0, "Primary key required in object used to update");

            Buffer qb(32);
            qb << "UPDATE " << mTable << " SET ";
            bool first{true};
            int index{1};
            using _Temp = removeIgnoreFields<WithoutAutoIncrement>;
            auto values = iod::foreach2(_Temp{}) | [&](const auto& m) {
                if (!first) {
                    qb << ", ";
                }
                first = false;
                qb << m.symbol().name() << " = ";
                Connection::params(qb, index++);

                return m.symbol() = m.symbol().member_access(o);
            };

            qb << " WHERE ";
            first = true;
            auto pks = iod::foreach2(pk) | [&](const auto& m) {
                if (!first) {
                    qb << " AND ";
                }
                first = false;
                qb << m.symbol().name() << " = ";
                Connection::params(qb, index++);

                return m.symbol() = m.symbol().member_access(o);
            };

            auto req = conn(qb);
            iod::apply(values, pks, req);

            return req.status();
        }

        template <typename T>
            requires iod::IsMetaType<T>
        void remove(const T& o)
        {
            Buffer qb(32);

            qb << "DELETE FROM " << mTable << " WHERE ";
            bool first{true};
            int index{1};
            auto values = iod::foreach(PrimaryKeys()) | [&](const auto& m) {
                if (!first) {
                    qb << " and ";
                }
                first = false;
                qb << m.symbol().name() << " = ";
                Connection::params(qb, index++);

                return m.symbol() = m.symbol().member_access(o);
            };

            // execute query
            iod::apply(values, conn(qb));
        }

        template <typename Column>
            requires (!iod::IsMetaType<Column> and IsQueryColumn<Column>)
        void remove(const Column& column)
        {
            Buffer qb(32);

            qb << "DELETE FROM " << mTable << " WHERE " << column.left.name() << " = ";
            Connection::params(qb, 1);
            conn(qb)(column.right);
        }

        ~Orm() {
            conn.put();
        }

    private:
        String mTable{};
        Connection& conn;
    };
}
#endif //SUILDB_ORM_HPP
