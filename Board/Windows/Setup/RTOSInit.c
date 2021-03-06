/*********************************************************************
*                     SEGGER Microcontroller GmbH                    *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 1995 - 2019 SEGGER Microcontroller GmbH                  *
*                                                                    *
*       Internet: segger.com  Support: support_embos@segger.com      *
*                                                                    *
**********************************************************************
*                                                                    *
*       embOS * Real time operating system for microcontrollers      *
*                                                                    *
*       Please note:                                                 *
*                                                                    *
*       Knowledge of this file may under no circumstances            *
*       be used to write a similar product or a real-time            *
*       operating system for in-house use.                           *
*                                                                    *
*       Thank you for your fairness !                                *
*                                                                    *
**********************************************************************
*                                                                    *
*       OS version: 5.06.1                                           *
*                                                                    *
**********************************************************************

-------------------------- END-OF-HEADER -----------------------------
Purpose : Initializes and handles the hardware for embOS
*/

#include <Windows.h>
#include <stdio.h>
#include <time.h>
#include <BSP.h>
#include <OSAL.h>
#include <OSAL_Clock.h>

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

#define TICK_IN_MS 1 /* 1 millisecond */ 

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
*
*       STATIC VARIABLES
*
**********************************************************************
*/
static  LARGE_INTEGER TampStart;
static  LARGE_INTEGER TampFreq;

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _ISRTickThread()
*/
static void _ISRTickThread(void) {
  int tDiffMax;
  int tDiff;
  int t;
  int tLast;

  //
  // If between two iterations more than tDiffMax ticks have elapsed,
  // then tDiff will be set to tDiffMax. This might occure when one
  // debugs and single steps through the application. This value
  // can be changed as desired.
  //
  tDiffMax = 1;
  //
  // Switch to higher timer resolution
  //
  timeBeginPeriod(1);
  tLast = timeGetTime();
  while (1) {
    t     = timeGetTime();
    tDiff = t - tLast;
    tLast = t;
    //
    // Limit the time to a certain amount of ms so that single
    // stepping does not mess things up to badly
    //
    if (tDiff > tDiffMax) {
      tDiff = tDiffMax;
    }
    //
    // Execute the actual ISR one time for every ms
    //

    while (tDiff-- > 0) {
      osalAdjustTimer(TICK_IN_MS);
    }

    //
    // SleepEx()'s second parameter *MUST* be TRUE when used with QueueUserAPC. Otherwise 'Nonpaged Pool'
    // (cf. ProcessExplorer) is congested *COMPLETELY* since we are NOT in an alertable state and thus the
    // queue will NEVER be flushed.
    //
    SleepEx(INFINITE, TRUE);
  }
}

/*********************************************************************
*
*       _voidAPC()
*
*  Function description
*    Dummy APC function. Is required because we (ab)use the
*    WIN32 QueueUserAPC() API function to wake up a thread
*/
static void APIENTRY _voidAPC(DWORD Dummy) {
  OS_USEPARA(Dummy);
}

/*********************************************************************
*
*       _CbSignalTickProc()
*
*  Function description
*    Timer callback function which periodically queues an APC in order
*    to resume the ISR tick thread.
*/
static void CALLBACK _CbSignalTickProc(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2) {
  OS_USEPARA(uID);
  OS_USEPARA(uMsg);
  OS_USEPARA(dw1);
  OS_USEPARA(dw2);
  QueueUserAPC(_voidAPC, (void*)dwUser, 0);
}

/*********************************************************************
*
*       TimesTampInit()
*
*/
static void TimesTampInit(void)
{
    // get count Frequency
    QueryPerformanceFrequency(&TampFreq);

    // get initial time
    QueryPerformanceCounter(&TampStart);
}

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*       OS_InitHW()
*
*  Function description
*    Initialize the hardware required for embOS to run.
*/
void OS_InitHW(void) {
  HANDLE hISRThread;

  TimesTampInit();

  SetThreadPriorityBoost(GetCurrentThread(), TRUE);  // Disable Windows priority boosting
  //
  // Start tick ISR
  //
  hISRThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) _ISRTickThread, NULL, 0, NULL);

  timeSetEvent(1,0, _CbSignalTickProc, (int)hISRThread, (TIME_PERIODIC | TIME_CALLBACK_FUNCTION));
}

/*********************************************************************
*
*       macMcuPrecisionCount()
* 
*/
OS_U32 macMcuPrecisionCount(void)
{
    LARGE_INTEGER count_end;

    // get end count
    QueryPerformanceCounter(&count_end);

    // get time (1000 000 us / 320us ) = 3125
    return (OS_U32)((count_end.QuadPart - TampStart.QuadPart) / (TampFreq.QuadPart / 3125.0));
}

/*********************************************************************
*
*       GetSecondTamp()
* 
*/
OS_U32 GetSecondTamp(void)
{
    LARGE_INTEGER count_end;

    // get end count
    QueryPerformanceCounter(&count_end);

    // get time
    return (OS_U32)((count_end.QuadPart - TampStart.QuadPart) / (TampFreq.QuadPart / 1.0));
}

/*********************************************************************
*
*       GetMillisecondTamp()
* 
*/
OS_U32 GetMillisecondTamp(void)
{
    LARGE_INTEGER count_end;

    // get end count
    QueryPerformanceCounter(&count_end);

    // get time
    return (OS_U32)((count_end.QuadPart - TampStart.QuadPart) / (TampFreq.QuadPart / 1000.0));
}

/*********************************************************************
*
*       GetMicrosecondTamp()
* 
*/
OS_U32 GetMicrosecondTamp(void)
{
    LARGE_INTEGER count_end;

    // get end count
    QueryPerformanceCounter(&count_end);

    // get time
    return (OS_U32)((count_end.QuadPart - TampStart.QuadPart) / (TampFreq.QuadPart / 1000000.0));
}

/*************************** End of file ****************************/
