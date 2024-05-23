package main

import (
    "io"
    "log"
    "os"
)

func main() {
    // 源文件路径和目标文件路径
    srcFile := "/usr/local/nginx/logs/111.log"
    dstFile := "/tmp/111.log"

    // 打开源文件
    src, err := os.Open(srcFile)
    if err != nil {
        log.Fatalf("无法打开源文件: %v", err)
    }
    defer src.Close()

    // 创建目标文件
    dst, err := os.Create(dstFile)
    if err != nil {
        log.Fatalf("无法创建目标文件: %v", err)
    }
    defer dst.Close()

    // 使用 io.Copy 进行文件拷贝
    bytesCopied, err := io.Copy(dst, src)
    if err != nil {
        log.Fatalf("文件拷贝失败: %v", err)
    }

    log.Printf("文件拷贝成功，拷贝了 %d 字节", bytesCopied)
}

