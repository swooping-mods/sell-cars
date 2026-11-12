#ifndef MOON_SELL_CARS_TRANSACTION_H
#define MOON_SELL_CARS_TRANSACTION_H
/* Callbacks return 1 only for a confirmed effect, 0 for a confirmed refusal,
 * and -1 when the effect is unknown. Unknown credit must never restore a car
 * for another payout. The process latch deliberately survives world changes. */
typedef struct {
 int (*remove)(void *), (*credit)(void *), (*restore)(void *), (*finish)(void *);
} MoonSaleOps;
enum MoonSaleResult {
 MOON_SALE_BLOCKED, MOON_SALE_REMOVE_REFUSED, MOON_SALE_REMOVE_UNKNOWN,
 MOON_SALE_CREDIT_RESTORED, MOON_SALE_RESTORE_UNKNOWN,
 MOON_SALE_CREDIT_UNKNOWN, MOON_SALE_CLEANUP_UNKNOWN, MOON_SALE_COMPLETE
};
static inline enum MoonSaleResult moon_sale_commit(const MoonSaleOps *ops,
                                                  void *context, int *blocked) {
 if(!blocked||*blocked||!ops||!ops->remove||!ops->credit||!ops->restore||!ops->finish)
  return MOON_SALE_BLOCKED;
 int removed=ops->remove(context);
 if(removed==0)return MOON_SALE_REMOVE_REFUSED;
 if(removed!=1){*blocked=1;return MOON_SALE_REMOVE_UNKNOWN;}
 int paid=ops->credit(context);
 if(paid==0){
  if(ops->restore(context)==1)return MOON_SALE_CREDIT_RESTORED;
  *blocked=1;return MOON_SALE_RESTORE_UNKNOWN;
 }
 if(paid!=1){*blocked=1;return MOON_SALE_CREDIT_UNKNOWN;}
 if(ops->finish(context)!=1){*blocked=1;return MOON_SALE_CLEANUP_UNKNOWN;}
 return MOON_SALE_COMPLETE;
}
#endif
