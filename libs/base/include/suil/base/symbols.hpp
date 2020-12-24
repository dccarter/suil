//
// Created by Mpho Mbotho on 2020-10-13.
//

#ifndef SUIL_BASE_SYMBOLS_HPP
#define SUIL_BASE_SYMBOLS_HPP

#include <iod/symbol.hh>

#ifndef IOD_SYMBOL_verbose
#define IOD_SYMBOL_verbose
    iod_define_symbol(verbose)
#endif

#ifndef IOD_SYMBOL_writer
#define IOD_SYMBOL_writer
    iod_define_symbol(writer)
#endif

#ifndef IOD_SYMBOL_format
    #define IOD_SYMBOL_format
    iod_define_symbol(format)
#endif

#ifndef IOD_SYMBOL_name
    #define IOD_SYMBOL_name
    iod_define_symbol(name)
#endif

#ifndef IOD_SYMBOL_unwire
    #define IOD_SYMBOL_unwire
    iod_define_symbol(unwire)
#endif

#ifndef IOD_SYMBOL_root
    #define IOD_SYMBOL_root
    iod_define_symbol(root)
#endif

#ifndef IOD_SYMBOL_onStdError
    #define IOD_SYMBOL_onStdError
    iod_define_symbol(onStdError)
#endif

#ifndef IOD_SYMBOL_onStdOut
    #define IOD_SYMBOL_onStdOut
    iod_define_symbol(onStdOut)
#endif

#endif //SUIL_BASE_SYMBOLS_HPP
