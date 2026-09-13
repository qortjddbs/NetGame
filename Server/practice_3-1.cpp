#include "Common.h"

bool IsLittleEndian()
{
	u_short x = 0x1234;
	if (x != htons(x)) return true;
	else return false;
}

bool IsBigEndian()
{
	u_short x = 0x1234;
	if (x == htons(x)) return true;
	else return false;
}

int main(int argc, char* argv[])
{
	if (IsLittleEndian() == true) {
		printf("Host uses LittleEndian\n");
	} 
	else if (IsBigEndian() == true) {
		printf("Host uses BigEndian\n");
	}

	return 0;
}
