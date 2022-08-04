# ccache 编译缓存,暂时还是玩具

## build

```shell
brew install opensll boost

git clone https://github.com/0x1306a94/ccache
cd ccache

mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ../
make

# run sample project
cd ../samples/ccache-sample
pod install
xed .
```

## 可以通过`NFS`进行局域网内缓存共享
