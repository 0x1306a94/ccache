# ccache 编译缓存,暂时还是玩具

## build

```shell
brew install opensll boost

git clone https://github.com/0x1306a94/ccache
cd ccache
./gen_xcode.sh
xed xcode_project
```

## usage
* 以[QMUIDemo_iOS](https://github.com/QMUI/QMUIDemo_iOS)为例, 首先下载源码
* 在`QMUIDemo_iOS`根目录创建`ccache`目录
* 然后在`ccache`目录下创建`ccache-clang` `ccache-clang++` 文件
* `ccache-clang` `ccache-clang++`文件内容如下
```shell
#!/bin/sh

# ccache-clang
CUR_DIR=$(dirname $0)
export CCACHE_DIR=$(dirname $0)/caches
export PATH=$CUR_DIR:$PATH
mkdir -p $CCACHE_DIR
env

# /usr/bin/clang "$@"
ccache /usr/bin/clang "$@"

#!/bin/sh

# ccache-clang++
CUR_DIR=$(dirname $0)
export CCACHE_DIR=$(dirname $0)/caches
export PATH=$CUR_DIR:$PATH
mkdir -p $CCACHE_DIR
env

# /usr/bin/clang++ "$@"
ccache /usr/bin/clang++ "$@"
```
* 赋予权限
```shell
chmod +x ccache-clang ccache-clang++
``` 
* `Xcode`配置 
  * 直接在`Build Settings` 点击 `+` 选择 `Add User-Defined Setting` 添加 `CC` 值为 `CC=$(SRCROOT)/ccache/ccache-clang` `CXX` 值为 `CXX=$(SRCROOT)/ccache/ccache-clang++`
  * 通过 `xcconfig` 配置
  ```shell
  CC=$(SRCROOT)/ccache/ccache-clang
  CXX=$(SRCROOT)/ccache/ccache-clang++
  ```
* 下图是清理缓存后首次编译
![](/imags/01.png)
* 清理缓存后使用缓存编译
![](/imags/02.png)
