#include<context.h>
#include<memory.h>
#include<lib.h>
extern u32 os_pfn_alloc(u32);
extern void os_pfn_free(u32, u64);
#define l4_SHIFT 39
#define l3_SHIFT 30
#define l2_SHIFT 21
#define l1_SHIFT 12
#define bit_mask 0x1FF
#define pres_bit_mask 0x5
#define pbit_mask 0x1
#define reserved 0x0000FFFFFFFFFF007


void gmp_data(u64 sa_stack, int lvl, u64 *pd_startAddr, u64 wrbit, int x, u32 apfn){
	if(lvl == 4){
		u64 l4_index = (sa_stack >> l4_SHIFT) & bit_mask;
		// SET THE READ/WRITE BIT FOR L4
		u64 wr = wrbit & 0x2;
		if (wr == 0x2) { pd_startAddr[l4_index] = pd_startAddr[l4_index] | 0x2;}
		
		if((pd_startAddr[l4_index] & pbit_mask) == 0){
			// SET THE U/S BIT and THE PRESENT BIT FOR L4
			pd_startAddr[l4_index] = pd_startAddr[l4_index] | pres_bit_mask;
			u32 l3_pfn = os_pfn_alloc(OS_PT_REG);
			u64 *l3 = (u64 *)osmap(l3_pfn);
			for(int i=0;i<512;i++){
				*(l3 + i) = 0x0000000000000000;
			}
			pd_startAddr[l4_index] = pd_startAddr[l4_index] | (l3_pfn << 12);
			pd_startAddr[l4_index] = pd_startAddr[l4_index] & reserved;
			gmp_data(sa_stack, 3, l3, wrbit,x, apfn);
			return;
		}
		else{
			u64 *l3 = (u64 *)osmap(((pd_startAddr[l4_index] >> 12) & 0x000000FFFFFFFFFF)); 
			gmp_data(sa_stack, 3, l3, wrbit, x,apfn);
			return;
		}

	}
	else if(lvl == 3){
		u64 l3_index = (sa_stack >> l3_SHIFT) & bit_mask;
		// SET THE READ/WRITE BIT FOR L3
		u64 wr = wrbit & 0x2;
		if (wr == 0x2) { pd_startAddr[l3_index] = pd_startAddr[l3_index] | 0x2;}
		if((pd_startAddr[l3_index] & pbit_mask) == 0){
			// SET THE U/S BIT and THE PRESENT BIT FOR L3
			pd_startAddr[l3_index] = pd_startAddr[l3_index] | pres_bit_mask;
			u32 l2_pfn = os_pfn_alloc(OS_PT_REG);
			u64 *l2 = (u64 *)osmap(l2_pfn);
			for(int i=0;i<512;i++){
				*(l2 + i) = 0x0000000000000000;
			}
			pd_startAddr[l3_index] = pd_startAddr[l3_index] | (l2_pfn << 12);
			pd_startAddr[l3_index] = pd_startAddr[l3_index] & reserved;
			gmp_data(sa_stack, 2, l2, wrbit,x, apfn);
			return;
		}
		else{
			u64 *l2 = (u64 *)osmap(((pd_startAddr[l3_index] >> 12) & 0x000000FFFFFFFFFF)); 
			gmp_data(sa_stack, 2, l2, wrbit,x, apfn); // REVERT 2 TO 3, IF NOT WORKING
			return;
		}
	}
	else if(lvl == 2){
		u64 l2_index = (sa_stack >> l2_SHIFT) & bit_mask;
		// SET THE READ/WRITE BIT FOR L2
		u64 wr = wrbit & 0x2;
		if (wr == 0x2) { pd_startAddr[l2_index] = pd_startAddr[l2_index] | 0x2;}

		if((pd_startAddr[l2_index] & pbit_mask) == 0){
			// SET THE U/S BIT and THE PRESENT BIT FOR L2
			pd_startAddr[l2_index] = pd_startAddr[l2_index] | pres_bit_mask;
			u32 l1_pfn = os_pfn_alloc(OS_PT_REG);
			u64 *l1 = (u64 *)osmap(l1_pfn);
			for(int i=0;i<512;i++){
				*(l1 + i) = 0x0000000000000000;
			}
			pd_startAddr[l2_index] = pd_startAddr[l2_index] | (l1_pfn << 12);
			pd_startAddr[l2_index] = pd_startAddr[l2_index] & reserved;
			gmp_data(sa_stack, 1, l1, wrbit, x,apfn);
			return;
		}
		else{
			u64 *l1 = (u64 *)osmap(((pd_startAddr[l2_index] >> 12) & 0x000000FFFFFFFFFF)); 
			gmp_data(sa_stack, 1, l1, wrbit, x,apfn);
			return;
		}
	}
	else{
		u64 l1_index = (sa_stack >> l1_SHIFT) & bit_mask;
		// SET THE READ/WRITE BIT FOR L1
		u64 wr = wrbit & 0x2;
		if (wr == 0x2) { pd_startAddr[l1_index] = pd_startAddr[l1_index] | 0x2;}
		
		if((pd_startAddr[l1_index] & pbit_mask) == 0){
			// SET THE U/S BIT and THE PRESENT BIT FOR L1
			pd_startAddr[l1_index] = pd_startAddr[l1_index] | pres_bit_mask;
			u32 data_sa;
			u64 *data;
			if(x == 1){
				data_sa = apfn;
				data = (u64 *)osmap(apfn);
			}
			else{
				data_sa = os_pfn_alloc(USER_REG);
				data = (u64 *)osmap(data_sa);
			}
			pd_startAddr[l1_index] = pd_startAddr[l1_index] | (data_sa << 12);
			
			return;
		}
	}

	return;
}


void gmp_cleaner(int c, u64 sa_stack, int lvl, u64 *pd_startAddr, u64 wrbit, int x, u32 apfn){
	if(lvl == 4){
		u64 l4_index = (sa_stack >> l4_SHIFT) & bit_mask;
		if((pd_startAddr[l4_index] & pbit_mask) == 1){
			u64 *l3 = (u64 *)osmap(((pd_startAddr[l4_index] >> 12) & 0x000000FFFFFFFFFF));
			
			if(lvl != c){
				gmp_cleaner(c,sa_stack, 3, l3, wrbit,x, apfn);return;
			}
			else{
				if(x==1){
					os_pfn_free(OS_PT_REG, (pd_startAddr[l4_index] >> 12) & 0x000000FFFFFFFFFF);
				}
			}
		}
	}
	else if(lvl == 3){
		u64 l3_index = (sa_stack >> l3_SHIFT) & bit_mask;
		if((pd_startAddr[l3_index] & pbit_mask) == 1){
			u64 *l2 = (u64 *)osmap(((pd_startAddr[l3_index] >> 12) & 0x000000FFFFFFFFFF)); 
			if(lvl != c){
				gmp_cleaner(c,sa_stack, 2, l2, wrbit,x, apfn);return;
			}
			else{
				os_pfn_free(OS_PT_REG, (pd_startAddr[l3_index] >> 12) & 0x000000FFFFFFFFFF);
			}
		}
		
	}
	else if(lvl == 2){
		u64 l2_index = (sa_stack >> l2_SHIFT) & bit_mask;
		if((pd_startAddr[l2_index] & pbit_mask) == 1){
			u64 *l1 = (u64 *)osmap(((pd_startAddr[l2_index] >> 12) & 0x000000FFFFFFFFFF)); 
			if(lvl != c){
				gmp_cleaner(c,sa_stack, 1, l1, wrbit, x,apfn);return;
			}
			else{
				os_pfn_free(OS_PT_REG, (pd_startAddr[l2_index] >> 12) & 0x000000FFFFFFFFFF);
				return;
			}
		}
		
	}
	else{
		u64 l1_index = (sa_stack >> l1_SHIFT) & bit_mask;
		if((pd_startAddr[l1_index] & pbit_mask) == 1){
			pd_startAddr[l1_index] = pd_startAddr[l1_index] & 0xFFFFFFFFFFFFFFFD;
			u32 l1_pfn = (pd_startAddr[l1_index] >> 12);
			os_pfn_free(USER_REG, l1_pfn);
			return;
		}
	}

	return;
}
void prepare_context_mm(struct exec_context *ctx)
{
	ctx->pgd = os_pfn_alloc(OS_PT_REG);
	u64 *l4 = (u64 *)osmap(ctx->pgd);
	for(int i=0;i<512;i++){
		*(l4 + i) = 0x0;
	}
	gmp_data(ctx->mms[MM_SEG_STACK].end-0x1, 4, l4, ctx->mms[MM_SEG_STACK].access_flags, 0, ctx->arg_pfn );
	gmp_data(ctx->mms[MM_SEG_CODE].start, 4, l4, ctx->mms[MM_SEG_CODE].access_flags, 0, ctx->arg_pfn );
	gmp_data(ctx->mms[MM_SEG_DATA].start, 4, l4, ctx->mms[MM_SEG_DATA].access_flags, 1, ctx->arg_pfn );
   return;
}
void cleanup_context_mm(struct exec_context *ctx)
{
	u64 *l4 = (u64 *)osmap(ctx->pgd);
	for(int i=1;i<5;i++){
		gmp_cleaner(i, ctx->mms[MM_SEG_STACK].end-0x1, 4, l4, ctx->mms[MM_SEG_STACK].access_flags, 0, ctx->arg_pfn );
		gmp_cleaner(i, ctx->mms[MM_SEG_CODE].start, 4, l4, ctx->mms[MM_SEG_CODE].access_flags, 0, ctx->arg_pfn );
		gmp_cleaner(i, ctx->mms[MM_SEG_DATA].start, 4, l4, ctx->mms[MM_SEG_DATA].access_flags, 1, ctx->arg_pfn );
	}
	os_pfn_free(OS_PT_REG, ctx->pgd);
	return;
}