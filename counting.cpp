#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <immintrin.h>

#include <iostream>
#include <ctime>
#include <ratio>
#include <chrono>
#include <cmath>
#include <random>
#include <thread>

/* config options */
int thread_cnt = 16;
long long thread_inc_times = 1 << 16;
int iteropt = 3;
int tsx_try_times = 10;
int run_bench = 0x03;
uint counter;
uint last_counter;
pthread_mutex_t lock;
pthread_mutexattr_t mta;
uint mytid = 0;

static volatile int hle_lock;

/* verbose options */
bool PrintParsingArgs;

void* incUseMutex(void *arg)
{
  pthread_mutex_lock(&lock);
  counter += 1;
  //printf("in x: %u\n", counter);
  pthread_mutex_unlock(&lock);

  return NULL;
}



void* incUseHle(void* arg) {
  while (__atomic_exchange_n (&hle_lock, 1, __ATOMIC_ACQUIRE
                              | __ATOMIC_HLE_ACQUIRE))
    _mm_pause ();

  // work with the acquired lock
  ++counter;

  __atomic_clear (&hle_lock, __ATOMIC_RELEASE | __ATOMIC_HLE_RELEASE);
  return NULL;
}

/* Somehow some counts are missed. There are generally two situations:
 * 1. Normally some counts are not counted, but the performance look normal,
 * slightly worse than Mutex
 * 2. When print counter in the critical section or comment _xend() out,
 * it ran very slow but the result looks right
 */
void* incUseRtm(void *arg) {
  unsigned status = _xbegin();

  if (status == _XBEGIN_STARTED) {
    ++counter;
    _xend();
    return NULL;
  }

  // fallback to mutex.
  incUseMutex(NULL);
  return NULL;
}

void* loopInc(void* lock_type) {
  for (int i = 0; i < thread_inc_times; i++) {
    switch(*((int*)lock_type)) {
    case 0x01: {
      incUseMutex(NULL);
      break;
    }
    case 0x02: {
      incUseHle(NULL);
      break;
    }
    case 0x04: {
      incUseRtm(NULL);
      break;
    }
    }

    /* try sleep a while after each loop */
    // std::mt19937_64 eng{std::random_device{}()};  // or seed however you want
    // std::uniform_int_distribution<> dist{1, 20};
    // std::this_thread::sleep_for(std::chrono::milliseconds{dist(eng)});

    
  }
  return NULL;
}


void counting(int lock_type) {
  int err;
  int rc = pthread_mutexattr_init(&mta);
  rc = pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_DEFAULT);
  pthread_t* tid = new pthread_t[thread_cnt];
	
  if (pthread_mutex_init(&lock, &mta) != 0) {
    printf("\n mutex init failed\n");
    return;
  }

  using namespace std::chrono;

  high_resolution_clock::time_point t1 = high_resolution_clock::now();

  for (int i = 0; i < thread_cnt; i++) {
    err = pthread_create(&(tid[i]), NULL, &loopInc, &lock_type);		
		
    if (err != 0)
      printf("\ncan't create thread :[%s]", strerror(err));
  }


  for (int i = 0; i < thread_cnt; i++) {
    pthread_join(tid[i], NULL);
  }

  pthread_mutex_destroy(&lock);

		
  high_resolution_clock::time_point t2 = high_resolution_clock::now();

  duration<double> time_span = duration_cast<duration<double>>(t2 - t1);

  if (lock_type == 0x01) {
    std::cout << "Mutex took " << time_span.count() << " seconds.";
  }
  else if (lock_type == 0x02) {
    std::cout << "HLE took " << time_span.count() << " seconds.";
  }
  else if (lock_type == 0x04) {
    std::cout << "RTM took " << time_span.count() << " seconds.";
  }
	
  std::cout << std::endl;
		
  printf("shared counter: %d\n", counter);
  return;
}

void iterBench(int lock_type) {
  for (int i = 0; i < iteropt; i++) {
    counting(lock_type);
    counter = 0;
  }
}

void benchCounting() {
  int runmask = run_bench;
  if (PrintParsingArgs) {
    printf("mask: %d\n", runmask);
  }
  int mutexcode = 0x01;
  int hlecode = 0x02;
  int rtmcode = 0x04;
  const int lock_type_num = 3;
  int codes[lock_type_num] = {mutexcode, hlecode, rtmcode};

  for (int i = 0; i < lock_type_num; i++) {
    if ((runmask & codes[i]) == codes[i]) {
      iterBench(codes[i]);
    }
  } 
}

void printHelp() {
  const char* msg =
    "Usage: ./counting [option]\n"
    "Description: compare the performance of pthread mutex, HLE and RTM with a counting benchmark.\n"
    "Examples: ./counting -t 4 -i 1 -c 5 -b 3\n"
    "Options:\n"
    "  -t, --thread-num=NUM \t\t\t specify the number of threads, defaults to 4\n"
    "  -i, --iteropt=NUM \t\t\t specify the number of iterations to run each test, defaults to 10\n"
    "  -r, --tsx-try-times=N \t\t\ try using tsx N times before falling back to mutex, defaults to 10\n"
    "  -c, --counter-max=N \t\t\t the shared counter will be incremented 2^N times by all threads, defaults to 2^20\n"
    "  -b, --run-bench=MASK \t\t\t use bit pattern to specify which bench to run, defaults to 0x03\n"
    "\t\t\t\t\t [0x01: test mutex, 0x02: test hle, 0x04: test rtm]\n"
    "  -v, --verbose[=parsing] \t\t make it the first option to print maximum verbose information\n"
    ;
  printf(msg);
}

void parseArgs(int argc, char** argv) {
  if (argc == 1) {
    printHelp();
    exit(0);
  }
	
  int c;

  while (1) {
    static struct option long_options[] =
      {
        /* These options set a flag. */
        // {"verbose", no_argument,       &verbose_flag, 1},
        // {"brief",   no_argument,       &verbose_flag, 0},
        /* These options don	int c;t set a flag.
           We distinguish them by their indices. */
        // {"add",     no_argument,       0, 'a'},
        // {"append",  no_argument,       0, 'b'},
        {"thread-num",       required_argument, 0, 't'},
        {"iteropt",          required_argument, 0, 'i'},
        {"tsx-try-times",    required_argument, 0, 'r'},
        {"thread-inc-count", required_argument, 0, 'c'},
        {"run-bench",        required_argument, 0, 'b'},
        {"verbose",          optional_argument, 0, 'v'},
        {"help",             no_argument, 0, 'h'},
        {0, 0, 0, 0}
      };
    /* getopt_long stores the option index here. */
    int option_index = 0;

    c = getopt_long (argc, argv, "t:i:r:b:c:v::h",
                     long_options, &option_index);

    /* Detect the end of the options. */
    if (c == -1)
      break;

    switch (c)
      {
      case 0:
        /* If this option set a flag, do nothing else now. */
        if (long_options[option_index].flag != 0)
          break;
        printf ("option %s", long_options[option_index].name);
        if (optarg)
          printf (" with arg %s", optarg);
        printf ("\n");
        break;

        // case 't':
        // 	puts ("option -a\n");
        // 	break;

      case 'v': {
        if (optarg == NULL) {
          /* enable all verbose flags */
          PrintParsingArgs = true;
        }
        else if (strcmp(optarg, "parsing") == 0) {
          PrintParsingArgs = true;
        }

        if (PrintParsingArgs)
          printf ("option -v with value `%s'\n", optarg);
        break;
      }
				
				
      case 't': {
        if (PrintParsingArgs)
          printf ("option -t with value `%s'\n", optarg);

        thread_cnt = std::stoi(optarg);
        break;
      }
				
      case 'b': {
        if (PrintParsingArgs)
          printf ("option -b with value `%s'\n", optarg);

        run_bench = std::stoi(optarg);
        break;
      }

      case 'i': {
        if (PrintParsingArgs)
          printf ("option -i with value `%s'\n", optarg);

        iteropt = std::stoi(optarg);
        break;
      }
				
				
      case 'r': {
        if (PrintParsingArgs)
          printf ("option -r with value `%s'\n", optarg);

        tsx_try_times = std::stoi(optarg);
        break;
      }
				

      case 'c': {
        if (PrintParsingArgs)
          printf ("option -c with value `%s'\n", optarg);

        thread_inc_times = std::pow(2, std::stoi(optarg));
        if (PrintParsingArgs)
          printf ("each thread increment %lld times\n", thread_inc_times);
        break;
      }
				

      case 'h': {
        printHelp();
        std::exit(0);
        break;
      }
				

      case '?':
        /* getopt_long already printed an error message. */
        break;

      default:
        abort ();
      }
  }


  /* Print any remaining command line arguments (not options). */
  if (optind < argc)
    {
      printf ("non-option ARGV-elements: ");
      while (optind < argc)
        printf ("%s ", argv[optind++]);
      putchar ('\n');
    }

}

int main(int argc, char** argv) {
  parseArgs(argc, argv);
  benchCounting();
  return 0;
}
