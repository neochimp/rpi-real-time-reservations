#include <stdlib.h>
#include <stdio.h>
#include <math.h>

int main(int argc, char **argv) {
  if (argc != 4) {
    printf("NaN");
    return NAN;
  }

  int term1 = atoi(argv[1]);
  int term2 = atoi(argv[3]);
  int valid = 1;
  int ans;

  switch (argv[2][0]) {
    case '+':
      ans = term1 + term2;
      break;
    case '-':
      ans = term1 - term2;
      break;
    case '*':
      ans = term1 * term2;
      break;
    case '/':
      if (term2 != 0)
        ans = term1 / term2;
      else
        valid = 0;
      break;
    default:
      valid = 0;
      break;
  }
  
  if (valid == 0) {
    printf("NaN");
    exit(1);
  }
  printf("%d", ans);
  
  return 0;
}
