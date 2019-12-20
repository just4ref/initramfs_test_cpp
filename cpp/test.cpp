#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

/*为了测试简单，这里不引入stl c++库*/

#define DEF_BUF_LEN 0x400000

struct map_vector {
  uint16_t x;
  uint16_t y;
};

struct point {
  uint32_t x;
  uint32_t y;
};

struct ioctl_data {
  uint16_t map_h;
  uint16_t map_w;
  uint16_t times;
  uint16_t nums;
  uint32_t points;
  uint32_t result;
};

class lacc_hw {
public:
  bool ready;
private:
  lacc_hw();
  int fd;
  char *buf;
  int buflen;
  struct ioctl_data idata;
  static lacc_hw *_instance;
public:
  bool init();
  int setup(int map_h, int map_w, int times, int nums, int points);
  void release();
  int compute();
  void fill_data();
  void dump_data(int bytes);
  static lacc_hw *instance();
  ~lacc_hw();
};

lacc_hw *lacc_hw::_instance = NULL;
lacc_hw::lacc_hw() {
  printf("construct\n");
  ready = false;
  buflen = DEF_BUF_LEN;
  memset((void *)(&idata), 0, sizeof(idata));
}

lacc_hw::~lacc_hw() {
  printf("deconstruct\n");
}

bool lacc_hw::init() {
  if (!ready) {
    fd = open("/dev/lacc", O_RDWR);
    buf = (char *)mmap(NULL, buflen, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_LOCKED, fd, 0);
    if (buf == MAP_FAILED) {
      printf("failed to mmap\n");
      return -1;
    }
  
    ready = true;
  }
  return true;
}

int lacc_hw::setup(int map_h, int map_w, int times, int nums, int points) {
  if (map_h*map_w > 801*801 || points > 360) {
    return -1;
  }
  idata.map_h = (uint16_t)map_h;
  idata.map_w = (uint16_t)map_w;
  idata.times = (uint16_t)times;
  idata.nums = (uint16_t)nums;
  idata.points = (uint32_t)points;
}

void lacc_hw::release() {
  if (ready) {
    munmap(buf, buflen);
    ready = false;
  }
}
int lacc_hw::compute() {
  int ret = 0;
  ret = ioctl(fd, 1000, &idata);
  printf("ioctl: ret=%d, result=%x\n", ret, idata.result);
  
  return 0;
}
void lacc_hw::fill_data() {
  srand(time(NULL));
  struct map_vector *pmv = (struct map_vector *)buf;
  struct point *pp = (struct point *)(buf + idata.map_h*idata.map_w*4);
  for (int i=0; i<idata.map_h*idata.map_w; i++) {
    pmv[i].x = (uint16_t)rand();
    pmv[i].y = (uint16_t)rand();
  }
  for (int i=0;i<idata.points;i++) {
    pp[i].x = rand();
    pp[i].y = rand();
  }
  
}
void lacc_hw::dump_data(int bytes) {
  int *pi = (int *)buf;
  if (bytes > buflen) bytes = buflen;
  for (int i=0; i<bytes/4;) {
    printf("%08x ", pi[i]);
    i++;
    if (i%8 == 0) {
      printf("\n");
    }
  }
}


lacc_hw *lacc_hw::instance() {
  if (_instance == NULL) {
    _instance = new lacc_hw();
  }
  return _instance;
}

int main() {
  lacc_hw *lacc = lacc_hw::instance();
  lacc->init();
  
  lacc->setup(801,801,1,4,360);
  lacc->fill_data();
  lacc->dump_data(1024);
  lacc->compute();
  
  lacc->release();
  return 0;
}

