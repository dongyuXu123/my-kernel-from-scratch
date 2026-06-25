# reference/ — 真实 Linux 源码(待下载)

本目录放**真实 v7.1 mainline 源码**,作为手写重构的唯一对照基准。

## 下载方法(在首个执行会话进行,本次脚手架会话不做)

```bash
cd ~/ZCodeProject/kernel-from-scratch/reference

# 方式 1: git clone(推荐,可查 blame/log)
git clone git://git.kernel.org/pub/linux/kernel/git/torvalds/linux.git linux-7.1
cd linux-7.1
git checkout v7.1

# 方式 2: 国内镜像加速
git clone https://mirrors.tuna.tsinghua.edu.cn/git/linux.git linux-7.1
cd linux-7.1
git checkout v7.1
```

## 验证下载正确

```bash
cd linux-7.1
head -5 Makefile
# 应看到 VERSION = 7  PATCHLEVEL = 1
git describe --tags
# 应输出 v7.1
```

## 重要约束

- **本目录只读** — 永不修改真实源码,只作对照
- 手写代码放 `../mykernel/`,不污染 reference
- 旧 AI 文档(`/run/media/dongyu/数据/.../*.md`)**不放这里**,它们是"待校准参考"
