#include <iostream>
#include <windows.h>
#include<string.h>
#include <tchar.h>

int main() {
    setlocale(LC_ALL, "");
    TCHAR szFileName[MAX_PATH];
    if (GetModuleFileName(NULL, szFileName, MAX_PATH)) {
        // 从完整路径中提取文件名部分
        LPCTSTR pszFileName = _tcsrchr(szFileName, '\\');
        if (pszFileName != NULL) {
            pszFileName++; // 移动到文件名的起始位置
            std::wcout << L"当亲"<<std::endl;
            std::wcout << L"当前进程的进程名为：" << pszFileName << std::endl;
        }
    }
    else {
        std::cerr << "获取进程名失败！" << std::endl;
        return 1;
    }
    return 0;
}
