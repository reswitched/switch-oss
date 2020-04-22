#include "nspr.h"
#include "prenv.h"
#include "nss.h"
#include "ssl.h"

#include <cstdlib>

#include "test_io.h"

#define GTEST_HAS_RTTI 0
#include "gtest/gtest.h"

std::string g_working_dir_path;

int main(int argc, char **argv) {
  // Start the tests
  ::testing::InitGoogleTest(&argc, argv);
  g_working_dir_path = ".";

  char* workdir = PR_GetEnvSecure("NSS_GTEST_WORKDIR");
  if (workdir)
    g_working_dir_path = workdir;

  for (int i = 0; i < argc; i++) {
    if (!strcmp(argv[i], "-d")) {
      g_working_dir_path = argv[i + 1];
      ++i;
    }
  }

  if (NSS_Initialize(g_working_dir_path.c_str(), "", "", SECMOD_DB,
                     NSS_INIT_READONLY) != SECSuccess) {
    return 1;
  }
  if (NSS_SetDomesticPolicy() != SECSuccess) {
    return 1;
  }
  int rv = RUN_ALL_TESTS();

  if (NSS_Shutdown() != SECSuccess) {
    return 1;
  }

  nss_test::Poller::Shutdown();

  return rv;
}
