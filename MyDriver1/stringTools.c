#include<ntddk.h>// For NT string functions

BOOLEAN CompareStrings(
    const void* str1,
    const void* str2,
    BOOLEAN str1IsUnicode,
    BOOLEAN str2IsUnicode,
    BOOLEAN caseInsensitive)
{
    // ��������ַ������� Unicode����ֱ�ӱȽ�
    if (str1IsUnicode && str2IsUnicode) {
        UNICODE_STRING us1, us2;
        RtlInitUnicodeString(&us1, (PCWSTR)str1);
        RtlInitUnicodeString(&us2, (PCWSTR)str2);

        return RtlEqualUnicodeString(&us1, &us2, caseInsensitive);
    }
    // ��������ַ������� ANSI����ֱ�ӱȽ�
    else if (!str1IsUnicode && !str2IsUnicode) {
        ANSI_STRING as1, as2;
        RtlInitAnsiString(&as1, (PCSZ)str1);
        RtlInitAnsiString(&as2, (PCSZ)str2);

        return RtlEqualString(&as1, &as2, caseInsensitive);
    }
    // ��� str1 �� ANSI��str2 �� Unicode
    else if (!str1IsUnicode && str2IsUnicode) {
        ANSI_STRING as1;
        UNICODE_STRING us2;
        WCHAR unicodeBuffer[256];  // ��ʱ����������ת��

        // ��ʼ�� ANSI �ַ���
        RtlInitAnsiString(&as1, (PCSZ)str1);

        // �� ANSI ת��Ϊ Unicode
        RtlAnsiStringToUnicodeString(&us2, &as1, TRUE);

        // �Ƚ�ת����� Unicode �ַ���
        BOOLEAN result = RtlEqualUnicodeString(&us2, (PCUNICODE_STRING)str2, caseInsensitive);

        // �ͷ�ת����� Unicode �ַ����ڴ�
        RtlFreeUnicodeString(&us2);
        return result;
    }
    // ��� str1 �� Unicode��str2 �� ANSI
    else if (str1IsUnicode && !str2IsUnicode) {
        ANSI_STRING as2;
        UNICODE_STRING us1, us2;
        WCHAR unicodeBuffer[256];  // ��ʱ����������ת��

        // ��ʼ�� ANSI �ַ���
        RtlInitAnsiString(&as2, (PCSZ)str2);

        // �� ANSI ת��Ϊ Unicode
        RtlAnsiStringToUnicodeString(&us2, &as2, TRUE);

        // ��ʼ�� Unicode �ַ��� str1
        RtlInitUnicodeString(&us1, (PCWSTR)str1);

        // �Ƚ�ת����� Unicode �ַ���
        BOOLEAN result = RtlEqualUnicodeString(&us1, &us2, caseInsensitive);

        // �ͷ�ת����� Unicode �ַ����ڴ�
        RtlFreeUnicodeString(&us2);
        return result;
    }
    return FALSE;
}
