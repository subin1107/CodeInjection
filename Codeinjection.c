#include <stdio.h>
#include <Windows.h>
#define _CRT_SECURE_NO_WARNINGS

typedef UINT(WINAPI* WINEXEC)(LPCSTR, UINT);  //�Լ� ������

typedef struct _THREAD_PARAM
{
    FARPROC pFunc[2]; //�����忡�� ����� �Լ����� �����͸� ����
    char szBuf[4][128]; //�����忡�� ����� ���ڿ����� ����
}THREAD_PARAM, * PTHREAD_PARAM;

//API���¸� typedef���� ��������� �ش� API�� ������ API�� �����ϰ� ����� ����
typedef HMODULE(WINAPI* PFLOADLIBRARYA)(LPCSTR lpLibFileName);
typedef FARPROC(WINAPI* PFGETPROCADDRESS)(HMODULE hModule, LPCSTR lpProcName);
typedef int (WINAPI* PFMESSAGEBOXA)(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);

DWORD WINAPI ThreadProc(LPVOID); //�Լ� ����
BOOL InjectCode(DWORD dwPID);

DWORD WINAPI ThreadProc(LPVOID lParam)
{
    PTHREAD_PARAM pParam = (PTHREAD_PARAM)lParam;
    HMODULE hMod = NULL;
    FARPROC pFunc = NULL;

    //��� ���μ��������� MessageBoxA �Լ� �ּҸ� �˾Ƴ��� ���� LoadLibraryA()�� ȣ��
    //MessageBoxA�� user32.dll�� ���� �ִ�. �� DLL�� ������ �ִ�.
    hMod = ((PFLOADLIBRARYA)pParam->pFunc[0])(pParam->szBuf[0]);

    //���� user32.dll���� MessageBoxA �Լ��� �ּҸ� ���´�.
    pFunc = (FARPROC)((PFGETPROCADDRESS)pParam->pFunc[1])(hMod, pParam->szBuf[1]);
    ((PFMESSAGEBOXA)pFunc)(NULL, pParam->szBuf[2], pParam->szBuf[3], MB_OK);

    return 0;
}
void AfterFunc() { };

BOOL InjectCode(DWORD dwPID) { //���������� �ڵ带 ������
    HMODULE hMod = NULL;
    THREAD_PARAM param = { 0, }; //�����忡 ���ڷ� ���� ����ü
    HANDLE hProcess = NULL;
    HANDLE hThread = NULL;
    LPVOID pRemoteBuf[2] = { 0, }; //��� ���μ����� �޸𸮿� Ȯ���� �������� �ּ�
    DWORD dwSize = 0;

    hMod = GetModuleHandleA("kernel32.dll");

    param.pFunc[0] = GetProcAddress(hMod, "LoadLibraryA"); //MessageBoxA�� ȣ���ϱ� ����
    param.pFunc[1] = GetProcAddress(hMod, "GetProcAddress"); //LoadLibraryA�Լ��� GetProcessAddress �Լ��� �ּ� ����
    strcpy(param.szBuf[0], "user32.dll");
    strcpy(param.szBuf[1], "MessageBoxA");

    strcpy(param.szBuf[2], "Hi"); //�޽��� �ڽ��� ���ڷ� �� ���ڿ���
    strcpy(param.szBuf[3], "Hello");

    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID);//��� ���μ����� ����
    dwSize = sizeof(THREAD_PARAM); //��� ���μ����� �� ������ ũ�⸦ ���Ѵ�
    pRemoteBuf[0] = VirtualAllocEx(hProcess, NULL, dwSize, MEM_COMMIT, PAGE_READWRITE); //��� ���μ����� �޸𸮿� ���� Ȯ��
    WriteProcessMemory(hProcess, pRemoteBuf[0], (LPVOID)&param, dwSize, NULL); //��� ���μ����� ������ ����
    dwSize = (DWORD)InjectCode - (DWORD)ThreadProc;//�ּ� ����� ���� �Լ��� ũ�⸦ ����
    pRemoteBuf[1] = VirtualAllocEx(hProcess, NULL, dwSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE); // ��� ���μ����� �޸� ������ Ȯ��
    WriteProcessMemory(hProcess, pRemoteBuf[1], (LPVOID)ThreadProc, dwSize, NULL); //��� ���μ����� ������ ����
    hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pRemoteBuf[1], pRemoteBuf[0], 0, NULL);
    //��� ���μ����� �޸𸮿� ���� ���� �ּҿ� ��� ���μ����� �޸𸮿� ���� �Լ� �ּҸ� �־� ������ ����

    WaitForSingleObject(hThread, INFINITE);

    CloseHandle(hThread);
    CloseHandle(hProcess);

    return TRUE;
}


int main(int argc, char* argv[]) {
    DWORD dwPID = 0;
    int getPID = 0;

    if (argc != 2) {
        printf("usage %s: pid\n", argv[0]);
        return 1;
    }


    printf("PID :");
    scanf("%d", &getPID);
    dwPID = (DWORD)getPID;
    InjectCode(dwPID);


    return 0;
}