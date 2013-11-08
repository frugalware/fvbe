#include <unistd.h>
#include <sys/reboot.h>

int main(void)
{
  sync();

  reboot(RB_POWER_OFF);

  return 0;
}
