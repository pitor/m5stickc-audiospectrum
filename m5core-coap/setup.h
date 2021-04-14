#ifndef __SETUP_HANDLING
#define __SETUP_HANDLING

enum setupStatus {
  CPSetup_OK,
  CPSetup_ReadFromSD,
  CPSetup_FailedReadingFromSD,
  CPSetup_ReadFromFlash,
  CPSetup_FailedReadingFromFlash,
};

setupStatus setupRead();
extern char wifi_ap[100];
extern char wifi_pass[100];

#endif