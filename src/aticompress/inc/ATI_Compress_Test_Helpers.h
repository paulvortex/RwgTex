// ATI_Compress_Test_Helpers.h : Defines the entry point for the console application.
//

#include "ATI_Compress.h"

ATI_TC_FORMAT GetFormat(DWORD dwFourCC);
DWORD GetFourCC(ATI_TC_FORMAT nFormat);
bool IsDXT5SwizzledFormat(ATI_TC_FORMAT nFormat);
ATI_TC_FORMAT ParseFormat(TCHAR* pszFormat);
TCHAR* GetFormatDesc(ATI_TC_FORMAT nFormat);

bool LoadDDSFile(TCHAR* pszFile, ATI_TC_Texture& texture);
void SaveDDSFile(TCHAR* pszFile, ATI_TC_Texture& texture, int mipMapCount);
