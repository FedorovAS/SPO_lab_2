#define WINVER 0x0502
#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <sddl.h>
#include <string>
#include <cstring>
using namespace std;


static PSECURITY_DESCRIPTOR create_security_descriptor()
{
    const char* sddl = "D:(A;OICI;GRGW;;;AU)(A;OICI;GA;;;BA)";
    PSECURITY_DESCRIPTOR security_descriptor = NULL;
    ConvertStringSecurityDescriptorToSecurityDescriptor(sddl, SDDL_REVISION_1, &security_descriptor, NULL);
    return security_descriptor;
}


static SECURITY_ATTRIBUTES create_security_attributes()
{
    SECURITY_ATTRIBUTES attributes;
    attributes.nLength = sizeof(attributes);
    attributes.lpSecurityDescriptor = create_security_descriptor();
    attributes.bInheritHandle = FALSE;
    return attributes;
}


BOOL WINAPI makemail(LPCTSTR mailname, HANDLE& mail, bool& serverstat)
{
    auto attributes = create_security_attributes();
    mail = CreateMailslot(mailname, 0, MAILSLOT_WAIT_FOREVER, &attributes);

    if (mail == INVALID_HANDLE_VALUE)
    {
        auto err = GetLastError();

        if (err == ERROR_INVALID_NAME || err == ERROR_ALREADY_EXISTS)
        {
            serverstat = FALSE;
            mail = CreateFile(mailname, GENERIC_WRITE, FILE_SHARE_READ, (LPSECURITY_ATTRIBUTES)NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);

            if (mail == INVALID_HANDLE_VALUE)
            {
                auto err = GetLastError();
                printf("Ошибка подключения! Код ошибки: %d\n", GetLastError());
                return FALSE;
            }
        }
        else
        {
            printf("Ошибка создания ящика с таким наименованием! Код ошибки: %d\n", GetLastError());
            return FALSE;
        }
    }
    else
        serverstat = TRUE;

    return TRUE;
}


void writemess(HANDLE mail, LPCTSTR lmess)
{
    DWORD bwrite;
    WriteFile(mail, lmess, (lstrlen(lmess) + 1) * sizeof(TCHAR), &bwrite, (LPOVERLAPPED)NULL);
    printf("Сообщение отправленно успешно!\n\n");
}


void checkmess(HANDLE mail, DWORD* bmess, DWORD* cmess)
{
    DWORD bmessl, cmessl;
    GetMailslotInfo(mail, (LPDWORD)NULL, &bmessl, &cmessl, (LPDWORD)NULL);
    printf("Найдено новых сообщений: %i!\n\n", cmessl);
}


void readmess(HANDLE mail)
{
    DWORD bmess, cmess, bread;
    BOOL fResult;
    bmess = cmess = bread = 0;
    fResult = GetMailslotInfo(mail, NULL, &bmess, &cmess, NULL);

    if (bmess == MAILSLOT_NO_MESSAGE)
    {
        printf("Новых сообщений нет!\n\n");
        return;
    }

    string mess(bmess, '\0');
    fResult = ReadFile(mail, &mess[0], sizeof(mess), &bread, NULL);
    cout << mess;
    return;
}


int main()
{
    setlocale(LC_ALL, "Russian");
    DWORD* bmess;
    DWORD* cmess;
    bmess = cmess = 0;
    string sname;
    printf("Введите полное наименование почтового ящика:");
    cin >> sname;
    LPCTSTR mailname = sname.c_str();
    HANDLE mail;
    bool serverstat;
    bool result = makemail(mailname, mail, serverstat);
    if (result == FALSE)
        return 0;

    printf((serverstat) ? "-------Сервер-------\n" : "-------Клиент-------\n");
    string command;

    while (TRUE)
    {
        printf((serverstat) ? "Перечень команд:\n   Введите <r> чтобы прочитать сообщение.\n" : "Перечень команд:\n   Введите <w> чтобы написать сообщение.\n");
        printf("   Введите <c> чтобы проверить наличие новых сообщений.\n");
        printf("   Введите <q> для выхода.\n\n"":");
        cin >> command;
        printf("\n");
        if (command == "c")
        {
            checkmess(mail, bmess, cmess);
        }
        else if (command == "q")
        {
            CloseHandle(mail);
            return 0;
        }
        else if ((command == "r") && (serverstat))
        {
            readmess(mail);
        }
        else if ((command == "w") && (!serverstat))
        {
            printf("Введите текст сообщения. Чтобы закончить сообщение, оставьте строку пустой.\n");
            string message, str;
            getline(cin, str);
            do
            {
                getline(cin, str);
                message += str + '\n';
            } while (!str.empty());
            writemess(mail, (LPCTSTR)message.c_str());
        }
        else if ((command != "w") || (command != "c") || (command != "r") || (command == "q"))
        {
            printf("Неизвестная команда. Введите другую команду.\n\n");
        }
    }
    return 0;
}