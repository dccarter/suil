FROM suilteam/remote:ubuntu

# Install ZSH
RUN apt-get -y update && apt-get -y upgrade
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y zsh

# Add user and home directory
RUN useradd -rm -d /home/dev -s /bin/zsh -g root -G sudo -u 1001 dev
USER dev
WORKDIR /home/dev

RUN sh -c "$(wget https://raw.githubusercontent.com/robbyrussell/oh-my-zsh/master/tools/install.sh -O -)"
