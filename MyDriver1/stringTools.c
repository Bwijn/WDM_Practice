#include<ntddk.h>// For NT string functions

BOOLEAN CompareStrings(
    const void* str1,
    const void* str2,
    BOOLEAN str1IsUnicode,
    BOOLEAN str2IsUnicode,
    BOOLEAN caseInsensitive)
{
    // 如果两个字符串都是 Unicode，则直接比较
    if (str1IsUnicode && str2IsUnicode) {
        UNICODE_STRING us1, us2;
        RtlInitUnicodeString(&us1, (PCWSTR)str1);
        RtlInitUnicodeString(&us2, (PCWSTR)str2);

        return RtlEqualUnicodeString(&us1, &us2, caseInsensitive);
    }
    // 如果两个字符串都是 ANSI，则直接比较
    else if (!str1IsUnicode && !str2IsUnicode) {
        ANSI_STRING as1, as2;
        RtlInitAnsiString(&as1, (PCSZ)str1);
        RtlInitAnsiString(&as2, (PCSZ)str2);

        return RtlEqualString(&as1, &as2, caseInsensitive);
    }
    // 如果 str1 是 ANSI，str2 是 Unicode
    else if (!str1IsUnicode && str2IsUnicode) {
        ANSI_STRING as1;
        UNICODE_STRING us2;
        WCHAR unicodeBuffer[256];  // 临时缓冲区用于转换

        // 初始化 ANSI 字符串
        RtlInitAnsiString(&as1, (PCSZ)str1);

        // 将 ANSI 转换为 Unicode
        RtlAnsiStringToUnicodeString(&us2, &as1, TRUE);

        // 比较转换后的 Unicode 字符串
        BOOLEAN result = RtlEqualUnicodeString(&us2, (PCUNICODE_STRING)str2, caseInsensitive);

        // 释放转换后的 Unicode 字符串内存
        RtlFreeUnicodeString(&us2);
        return result;
    }
    // 如果 str1 是 Unicode，str2 是 ANSI
    else if (str1IsUnicode && !str2IsUnicode) {
        ANSI_STRING as2;
        UNICODE_STRING us1, us2;
        WCHAR unicodeBuffer[256];  // 临时缓冲区用于转换

        // 初始化 ANSI 字符串
        RtlInitAnsiString(&as2, (PCSZ)str2);

        // 将 ANSI 转换为 Unicode
        RtlAnsiStringToUnicodeString(&us2, &as2, TRUE);

        // 初始化 Unicode 字符串 str1
        RtlInitUnicodeString(&us1, (PCWSTR)str1);

        // 比较转换后的 Unicode 字符串
        BOOLEAN result = RtlEqualUnicodeString(&us1, &us2, caseInsensitive);

        // 释放转换后的 Unicode 字符串内存
        RtlFreeUnicodeString(&us2);
        return result;
    }
    return FALSE;
}
