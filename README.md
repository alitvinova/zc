# ZCash source code research
------------------

Чтобы собрать приложение, использующее кодовую базу ZCash, нужно выполнить следующие шаги:
1. Первые шаги скопированы отсюда: https://github.com/zcash/zcash/wiki/1.0-User-Guide#install-dependencies
Здесь описаны действия только для Ubuntu/Debian.
Install dependencies

```$ sudo apt-get install \
      build-essential pkg-config libc6-dev m4 g++-multilib \
      autoconf libtool ncurses-dev unzip git python python-zmq \
      zlib1g-dev wget bsdmainutils automake```

Check gcc version

gcc/g++ 4.9 or later is required. Currently there is a bug preventing use of gcc/g++ 7.x. Use g++ --version to check which version you have.

On Ubuntu Trusty, you can install gcc/g++ 4.9 as follows:

```$ sudo add-apt-repository ppa:ubuntu-toolchain-r/test
$ sudo apt-get update
$ sudo apt-get install g++-4.9```

Check binutils version

binutils 2.22 or later is required. Use as --version to check which version you have, and upgrade if necessary.

Fetch the software and parameter files

Fetch our repository with git and run fetch-params.sh like so:

```$ git clone https://github.com/zcash/zcash.git
$ cd zcash/
$ git checkout v1.0.11
$ ./zcutil/fetch-params.sh```

2. Скопировать build.sh в zcash/zcutil
   Скопировать остальные файлы в zcash/src

$ ```./zcutil/build.sh --disable-rust --enable-wallet --disable-zmq -j$(nproc)```

3. Для изменения параметров сборки tx_creator редактируем zcash/src/Makefile.am:

559 tx_creator_SOURCES = dummysender.cpp txcreator.cpp <---------------------вот тут указываем, какие cpp файлы компилить

4. Для внесения изменений в бинарный файл tx_creator редактируем 
  dummysender.cpp		
  dummysender.h		
  dummytxcreator.cpp	
  txcreator.cpp

5. Подробнее, что где.
Если ничего не менять в Makefile.am, то собираться будет следующее:
dummysender.cpp
dummysender.h
txcreator.cpp

Это код из z_sendmany(tx_creator) и rpcwallet(dummysender).

Во время исследования кода появлялось больше понимания, как архитектуры библиотеки, так и задачи в рамках текущего проекта.
В dummytxcreator.cpp отдельно запускаются функции, например, ZCJoinSplit::Generate(). Чтобы собрать бинарный файл с этой функцией, меняем в zcash/src/Makefile.am:
559 tx_creator_SOURCES = dummysender.cpp txcreator.cpp
на
559 tx_creator_SOURCES = dummytxcreator.cpp

После чего идем в zcash/src и набираем
$make

tx_creator запускается без параметров.
