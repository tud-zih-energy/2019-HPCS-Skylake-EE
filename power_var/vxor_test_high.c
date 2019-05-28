/*   Test program that triggers AVX-512 workload on different data
*
*    Copyright (C) 2019  TU Dresden
*
*    This program is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <scorep/SCOREP_User.h>

SCOREP_USER_METRIC_LOCAL( my_local_metric )
SCOREP_USER_METRIC_LOCAL( my_local_metric_2 )

#define PREFIX "v"
#define POSTFIX "64"
#define REGISTER "z"


void calculate(unsigned long long addr[16], unsigned long long passes);


void main()
{
  unsigned long long addr[16];
  unsigned long long passes=10000000000;
  unsigned report_every=8;
  unsigned long long i,a;
  unsigned long long value=0;
  unsigned long long value2=0;

  SCOREP_USER_REGION_DEFINE( my_region_handle )
  SCOREP_USER_REGION_INIT( my_region_handle, "my_region",SCOREP_USER_REGION_TYPE_COMMON )

  SCOREP_USER_METRIC_INIT( my_local_metric, "Set Bits in value", "#", \
                             SCOREP_USER_METRIC_TYPE_UINT64, \
                             SCOREP_USER_METRIC_CONTEXT_GLOBAL )
  SCOREP_USER_METRIC_INIT( my_local_metric_2, "Set Bits in value2", "#", \
                             SCOREP_USER_METRIC_TYPE_UINT64, \
                             SCOREP_USER_METRIC_CONTEXT_GLOBAL )


  // warm up
  value=-1;
  value2=-1;
  for (i=0;i<8;i++)
     addr[i]=value;
  for (i=8;i<16;i++)
     addr[i]=value2;

  calculate(addr,passes*50);

  value=0;
  value2=0;
  for (int value_i=0;value_i<=64;value_i++)
  {
    if (value_i%report_every != 0)
    {
      value=value|(1ULL)<<value_i;
      continue;
    }
    SCOREP_USER_METRIC_INT64( my_local_metric, value_i )
    
    value2=0;

    for (int value2_i=0;value2_i<=64;value2_i++)
    {
      if (value2_i%report_every != 0)
      {
        value2=value2|(1ULL)<<value2_i;
        continue;
      }


      for (i=0;i<8;i++)
        addr[i]=value;
      for (i=8;i<16;i++)
        addr[i]=value2;

      SCOREP_USER_METRIC_INT64( my_local_metric_2, value2_i )

      SCOREP_USER_REGION_ENTER( my_region_handle )

      calculate(addr,passes);
      printf("sth done\n");

      SCOREP_USER_REGION_END( my_region_handle )

      value2=value2|(1ULL)<<value2_i;
    }
    value=value|(1ULL)<<value_i;
  }
}
/*
Here we will populate zmm0-7 with VALUE, zmm8-zmm15 with VALUE2
Then we will execute
zmm[8-15]=zmm[0-7]^zmm[8-15]
This will be done 4*`passes` (4 due to unrrolling) time
*/
void calculate(unsigned long long addr[16], unsigned long long passes)
{
#pragma omp parallel
  {
       __asm__ __volatile__(
           "mov %%rax,%%r9;"   // addr
           "mov %%rbx,%%r10;"  // passes
            PREFIX"movdqu"POSTFIX" 0(%%r9), %%"REGISTER"mm0;"
            PREFIX"movdqu"POSTFIX" 0(%%r9), %%"REGISTER"mm1;"
            PREFIX"movdqu"POSTFIX" 0(%%r9), %%"REGISTER"mm2;"
            PREFIX"movdqu"POSTFIX" 0(%%r9), %%"REGISTER"mm3;"
            PREFIX"movdqu"POSTFIX" 0(%%r9), %%"REGISTER"mm4;"
            PREFIX"movdqu"POSTFIX" 0(%%r9), %%"REGISTER"mm5;"
            PREFIX"movdqu"POSTFIX" 0(%%r9), %%"REGISTER"mm6;"
            PREFIX"movdqu"POSTFIX" 0(%%r9), %%"REGISTER"mm7;"
            PREFIX"movdqu"POSTFIX" 64(%%r9), %%"REGISTER"mm8;"
            PREFIX"movdqu"POSTFIX" 64(%%r9), %%"REGISTER"mm9;"
            PREFIX"movdqu"POSTFIX" 64(%%r9), %%"REGISTER"mm10;"
            PREFIX"movdqu"POSTFIX" 64(%%r9), %%"REGISTER"mm11;"
            PREFIX"movdqu"POSTFIX" 64(%%r9), %%"REGISTER"mm12;"
            PREFIX"movdqu"POSTFIX" 64(%%r9), %%"REGISTER"mm13;"
            PREFIX"movdqu"POSTFIX" 64(%%r9), %%"REGISTER"mm14;"
            PREFIX"movdqu"POSTFIX" 64(%%r9), %%"REGISTER"mm15;"
/*            "vmovdqu64 0(%%r9), %%zmm0;"
            "vmovqdu64 0(%%r9), %%zmm1;"
            "vmovqdu64 0(%%r9), %%zmm2;"
            "vmovqdu64 0(%%r9), %%zmm3;"
            "vmovqdu64 0(%%r9), %%zmm4;"
            "vmovqdu64 0(%%r9), %%zmm5;"
            "vmovqdu64 0(%%r9), %%zmm6;"
            "vmovqdu64 0(%%r9), %%zmm7;"
            "vmovqdu64 64(%%r9), %%zmm8;"
            "vmovqdu64 64(%%r9), %%zmm9;"
            "vmovqdu64 64(%%r9), %%zmm10;"
            "vmovqdu64 64(%%r9), %%zmm11;"
            "vmovqdu64 64(%%r9), %%zmm12;"
            "vmovqdu64 64(%%r9), %%zmm13;"
            "vmovqdu64 64(%%r9), %%zmm14;"
            "vmovqdu64 64(%%r9), %%zmm15;"*/
            "jmp _work_loop_avx_store_pi_8;"
            ".align 64,0x0;"
            "_work_loop_avx_store_pi_8:"
            "vxorps %%zmm0, %%zmm8, %%zmm8;"
            "vxorps %%zmm1, %%zmm9, %%zmm9;"
            "vxorps %%zmm2, %%zmm10, %%zmm10;"
            "vxorps %%zmm3, %%zmm11, %%zmm11;"
            "vxorps %%zmm4, %%zmm12, %%zmm12;"
            "vxorps %%zmm5, %%zmm13, %%zmm13;"
            "vxorps %%zmm6, %%zmm14, %%zmm14;"
            "vxorps %%zmm7, %%zmm15, %%zmm15;"
            "vxorps %%zmm0, %%zmm8, %%zmm8;"
            "vxorps %%zmm1, %%zmm9, %%zmm9;"
            "vxorps %%zmm2, %%zmm10, %%zmm10;"
            "vxorps %%zmm3, %%zmm11, %%zmm11;"
            "vxorps %%zmm4, %%zmm12, %%zmm12;"
            "vxorps %%zmm5, %%zmm13, %%zmm13;"
            "vxorps %%zmm6, %%zmm14, %%zmm14;"
            "vxorps %%zmm7, %%zmm15, %%zmm15;"
            "vxorps %%zmm0, %%zmm8, %%zmm8;"
            "vxorps %%zmm1, %%zmm9, %%zmm9;"
            "vxorps %%zmm2, %%zmm10, %%zmm10;"
            "vxorps %%zmm3, %%zmm11, %%zmm11;"
            "vxorps %%zmm4, %%zmm12, %%zmm12;"
            "vxorps %%zmm5, %%zmm13, %%zmm13;"
            "vxorps %%zmm6, %%zmm14, %%zmm14;"
            "vxorps %%zmm7, %%zmm15, %%zmm15;"
            "vxorps %%zmm0, %%zmm8, %%zmm8;"
            "vxorps %%zmm1, %%zmm9, %%zmm9;"
            "vxorps %%zmm2, %%zmm10, %%zmm10;"
            "vxorps %%zmm3, %%zmm11, %%zmm11;"
            "vxorps %%zmm4, %%zmm12, %%zmm12;"
            "vxorps %%zmm5, %%zmm13, %%zmm13;"
            "vxorps %%zmm6, %%zmm14, %%zmm14;"
            "vxorps %%zmm7, %%zmm15, %%zmm15;"
            "sub $32,%%r10;"
            "jnz _work_loop_avx_store_pi_8;"
            :
            : "a"(addr), "b" (passes)
            : "%r8", "%r9", "%r10", "%r11", "%r12", "%r13", "%r14", "%r15", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7", "xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15", "memory"
            );
  }
}
