#include "kernel/types.h"
#include "user.h"

void print_time(uint64 total_ns) {
    uint64 total_sec = total_ns / 1000000000ULL;
    uint ns = total_ns % 1000000000ULL;

    uint days = total_sec / 86400;
    uint day_time = total_sec % 86400;

    uint h = day_time / 3600;
    uint m = (day_time % 3600) / 60;
    uint s = day_time % 60;

    uint year = 1970;
    while (1) {
        uint days_in_year = ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) ? 366 : 365;
        if (days >= days_in_year) {
            days -= days_in_year;
            year++;
        } else {
            break;
        }
    }

    int is_v = ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
    uint days_in_month[] = {31, 28 + is_v, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    uint month = 0;
    while (1) {
        if (days >= days_in_month[month]) {
            days -= days_in_month[month];
            month++;
        } else {
            break;
        }
    }

    uint day = days + 1;
    month += 1;

    printf("%d-%d-%d %d:%d:%d.%d\n", year, month, day, h, m, s, ns);
}

int
main(int argc, char *argv[])
{
  uint64 time = get_time();
  print_time(time);
  exit(0);
}
