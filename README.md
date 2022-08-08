# ccache 编译缓存,暂时还是玩具

![](/images/process.png)

## build

```shell
brew install openssl boost xxhash

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

## usage
* 在项目根目录创建`ccache`目录
* 然后在`ccache`目录下创建`ccache-clang` `ccache-clang++` 文件
* `ccache-clang` `ccache-clang++`文件内容如下
```shell
#!/bin/sh

# ccache-clang
CUR_DIR=$(dirname $0)
CCACHE_EXEC_DIR=$CUR_DIR/../../../build
# 指定配置文件
export CCACHE_CONFIG=$CUR_DIR/../../config.yaml
# 启用trace记录,需要在编译时指定 -DMTR_ENABLED=ON
export CCACHE_INTERNAL_TRACE=1
export PATH=$CCACHE_EXEC_DIR:$PATH
# env

echo "LLBUILD_BUILD_ID: ${LLBUILD_BUILD_ID}"
echo "LLBUILD_TASK_ID: ${LLBUILD_TASK_ID}"

# /usr/bin/clang "$@"
ccache /usr/bin/clang "$@"

#!/bin/sh

# ccache-clang++
CUR_DIR=$(dirname $0)
CCACHE_EXEC_DIR=$CUR_DIR/../../../build
# 指定配置文件
export CCACHE_CONFIG=$CUR_DIR/../../config.yaml
# 启用trace记录,需要在编译时指定 -DMTR_ENABLED=ON
export CCACHE_INTERNAL_TRACE=1
export PATH=$CCACHE_EXEC_DIR:$PATH
# env

echo "LLBUILD_BUILD_ID: ${LLBUILD_BUILD_ID}"
echo "LLBUILD_TASK_ID: ${LLBUILD_TASK_ID}"

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
* 配置文件采用`YAML`格式
```yaml
# 缓存位置配置
file_storage:
  scheme: "file" # 使用文件缓存
  path: "/Users/${USER}/Work/NFSD_WORK/ccache_samples"
# 日志存放目录
log_dir: "/Users/${USER}/Work/NFSD_WORK/ccache_sample_log"
# 是否将日志输出到控制台,开启后可以在Xcode 编译日志中查看
console_log: true
# 计算缓存key是否以依赖文件修改时间
# key_by_time: true
# 计算缓存key是否以依赖文件内容
key_by_content: true
# 当参数种存在路径时,需要忽略的前缀
ignore_path_prefix:
  # - "/Applications/Xcode.app/Contents/Developer"
  - "/Users/${USER}/Library/Developer/Xcode/DerivedData/ccache-sample-"
  - "/Users/${USER}/Library/Developer/Xcode/DerivedData"
# 当参数种存在路径时,需要删除的前缀
remove_path_prefix:
  - "/Users/${USER}/WorkSpace/SourceCode/ccache/samples"
  - "/Users/${USER}/Work/NFSD_WORK/ccache/samples"

```
  
## 可以通过`NFS`进行局域网内缓存共享
* 如上述配置文件中`/Users/${USER}/Work/NFSD_WORK`
* `macOS nfs` 服务端配置, 配置文件为`/etc/exports`, 配置如下, 具体参数参见`man exports`
```shell
/Users/xxx/Work/NFSD_WORK -alldirs -mapall=501:20 -network=172.30.0.0 -mask=255.255.0.0
```
* `nfsd`命令
```shell
# 启用nfsd 默认已启用
sudo nfsd enable 
# 检查/etc/exports 是否合法
sudo nfsd checkexports
# 重新启动
sudo nfsd restart

# 查看是否正常共享
showmount -e
```
* `macOS nfs` 客户端配置
```shell
# 创建需要挂载为本就的路径
mkdir -p ~/Work/NFSD_WORK
# 将服务端目录挂载到本地目录
sudo mount -t nfs 172.30.101.2:/Users/xxx/Work/NFSD_WORK ~/Work/NFSD_WORK/ 
# 取消挂载
sudo umount ~/Work/NFSD_WORK/ 
```
## Thanks
* 部分代码借鉴于[ccache/ccache](https://github.com/ccache/ccache)
