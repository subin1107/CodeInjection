#include <stdio.h>
#include <Windows.h>
#define _CRT_SECURE_NO_WARNINGS

typedef UINT(WINAPI* WINEXEC)(LPCSTR, UINT);  //함수 포인터

typedef struct _THREAD_PARAM
{
    FARPROC pFunc[2]; //쓰레드에서 사용할 함수들의 포인터를 저장
    char szBuf[4][128]; //쓰레드에서 사용할 문자열들을 저장
}THREAD_PARAM, * PTHREAD_PARAM;

//API형태를 typedef으로 정의해줘야 해당 API를 기존의 API와 동일하게 사용이 가능
typedef HMODULE(WINAPI* PFLOADLIBRARYA)(LPCSTR lpLibFileName);
typedef FARPROC(WINAPI* PFGETPROCADDRESS)(HMODULE hModule, LPCSTR lpProcName);
typedef int (WINAPI* PFMESSAGEBOXA)(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);

DWORD WINAPI ThreadProc(LPVOID); //함수 선언
BOOL InjectCode(DWORD dwPID);

DWORD WINAPI ThreadProc(LPVOID lParam)
{
    PTHREAD_PARAM pParam = (PTHREAD_PARAM)lParam;
    HMODULE hMod = NULL;
    FARPROC pFunc = NULL;

    //대상 프로세스에서의 MessageBoxA 함수 주소를 알아내기 위해 LoadLibraryA()를 호출
    //MessageBoxA는 user32.dll에 속해 있다. 그 DLL을 얻어오고 있다.
    hMod = ((PFLOADLIBRARYA)pParam->pFunc[0])(pParam->szBuf[0]);

    //얻어온 user32.dll에서 MessageBoxA 함수의 주소를 얻어온다.
    pFunc = (FARPROC)((PFGETPROCADDRESS)pParam->pFunc[1])(hMod, pParam->szBuf[1]);
    ((PFMESSAGEBOXA)pFunc)(NULL, pParam->szBuf[2], pParam->szBuf[3], MB_OK);

    return 0;
}
void AfterFunc() { };

BOOL InjectCode(DWORD dwPID) { //본격적으로 코드를 인젝션
    HMODULE hMod = NULL;
    THREAD_PARAM param = { 0, }; //스레드에 인자로 넣을 구조체
    HANDLE hProcess = NULL;
    HANDLE hThread = NULL;
    LPVOID pRemoteBuf[2] = { 0, }; //대상 프로세스의 메모리에 확보한 데이터의 주소
    DWORD dwSize = 0;

    hMod = GetModuleHandleA("kernel32.dll");

    param.pFunc[0] = GetProcAddress(hMod, "LoadLibraryA"); //MessageBoxA를 호출하기 위해
    param.pFunc[1] = GetProcAddress(hMod, "GetProcAddress"); //LoadLibraryA함수와 GetProcessAddress 함수의 주소 구함
    strcpy(param.szBuf[0], "user32.dll");
    strcpy(param.szBuf[1], "MessageBoxA");

    strcpy(param.szBuf[2], "Hi"); //메시지 박스에 인자로 줄 문자열들
    strcpy(param.szBuf[3], "Hello");

    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID);//대상 프로세스를 연다
    dwSize = sizeof(THREAD_PARAM); //대상 프로세스에 쓸 내용의 크기를 구한다
    pRemoteBuf[0] = VirtualAllocEx(hProcess, NULL, dwSize, MEM_COMMIT, PAGE_READWRITE); //대상 프로세스의 메모리에 공간 확보
    WriteProcessMemory(hProcess, pRemoteBuf[0], (LPVOID)&param, dwSize, NULL); //대상 프로세스에 내용을 쓴다
    dwSize = (DWORD)InjectCode - (DWORD)ThreadProc;//주소 계산을 위해 함수의 크기를 구함
    pRemoteBuf[1] = VirtualAllocEx(hProcess, NULL, dwSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE); // 대상 프로세스의 메모리 공간을 확보
    WriteProcessMemory(hProcess, pRemoteBuf[1], (LPVOID)ThreadProc, dwSize, NULL); //대상 프로세스에 내용을 쓴다
    hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pRemoteBuf[1], pRemoteBuf[0], 0, NULL);
    //대상 프로세스의 메모리에 써준 인자 주소와 대상 프로세스의 메모리에 써준 함수 주소를 넣어 스레드 실행

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