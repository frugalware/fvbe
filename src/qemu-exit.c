#include <unistd.h>
#include <sys/reboot.h>

int main(void)
{
  reboot(RB_POWER_OFF);

  return 0;
}
