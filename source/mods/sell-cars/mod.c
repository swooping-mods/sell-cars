/* M18: remove one exact owned empty car and credit half its vanilla offer. */
#include "base.h"
#include "../../runtime/native/input.h"
#include "../vehicle-framework/sdk/fleet_adapter.h"
#include "transaction.h"
#include "../../runtime/native/local_actor.h"
static PTR index_m,offer_m,decimal_div,decimal_two,credit_m,exit_m,prepare_m,object_m,destroy_m,revision_m,set_revision_m,moved_m,save_m,cargo_m,dust_m,speed_m;
static PTR owned_f,garage_f,storage_f;
static DRoot pending_world,pending_vehicle,pending_key,pending_owned,pending_walker;
static float deadline,armed_at;
static int sales_blocked,sale_busy;
static void cancel(void){
 ddrop(&pending_world);ddrop(&pending_vehicle);ddrop(&pending_key);ddrop(&pending_owned);ddrop(&pending_walker);deadline=0;armed_at=0;
}
/* Dictionary presence and out-value must agree. Never remove an entry selected
 * only by model/buyer, or trust an out-value after a failed native lookup. */
static int entry(PTR dictionary,PTR key_obj,PTR *value){
 *value=0;if(!dictionary||!key_obj)return -1;
 int ok=1;void *a[]={key_obj,value};
 PTR result=call(get_method(object_class(dictionary),"TryGetValue",2),dictionary,a,&ok);
 int found=0;
 if(!ok||!moon_sale_bool_data(bdata(result),&found))return -1;
 if(found!=(*value!=0))return -1;
 return found;
}
static int exact(PTR dictionary,PTR key_obj,PTR expected){
 PTR value=0;return entry(dictionary,key_obj,&value)==1&&value==expected;
}
static int remove_exact(PTR dictionary,PTR key_obj,PTR expected){
 if(!exact(dictionary,key_obj,expected))return 0;
 int ok=1;void *a[]={key_obj};
 PTR result=call(get_method(object_class(dictionary),"Remove",1),dictionary,a,&ok);
 int removed=0;
 if(!ok||!moon_sale_bool_data(bdata(result),&removed))return -1;
 PTR after=0;
 int found=entry(dictionary,key_obj,&after);
 if(removed&&found==0)return 1;
 if(!removed&&found==1&&after==expected)return 0;
 return -1;
}
/* The snapshot value ABI/content contract is not yet qualified. Presence is
 * queried without an out-buffer; opaque persisted contents must not be deleted. */
static int snapshot_absent(PTR dictionary,PTR key_obj){
 if(!dictionary||!key_obj)return 0;
 int ok=1;void *a[]={key_obj};
 PTR result=call(get_method(object_class(dictionary),"ContainsKey",1),dictionary,a,&ok);
 int present=1;return ok&&moon_sale_bool_data(bdata(result),&present)&&!present;
}
static PTR selected(PTR world,PTR vehicle,PTR key_obj){
 int ok=1;PTR owned=mf_lookup(world,key_obj,&ok);
 MoonFleetRecord record={0};
 if(!ok||!owned||!mf_read(owned,&record)||!mf_equal(record.storage,key_obj))return 0;
 PTR local=call(mf.local,0,0,&ok);
 if(!ok||!moon_fleet_local(&mf_queries,&record,local)||!moon_fleet_unoccupied(&mf_queries,&record))return 0;
 int index=bint(index_m,vehicle,&ok);
 PTR dictionary=0;field_value(world,garage_f,&dictionary);
 return ok&&index==record.model&&exact(dictionary,key_obj,vehicle)?owned:0;
}
static int empty_stopped(PTR vehicle){
 int ok=1;int cargo=bint(cargo_m,vehicle,&ok);
 PTR dust=bdata(call(dust_m,vehicle,0,&ok));
 if(!ok||!dust)return 0;
 double kilograms=*(double*)dust;
 float speed=bnumber(speed_m,vehicle,&ok);
 /* Zero and range comparisons reject negative/NaN dust and NaN speed. */
 return ok&&cargo==0&&kilograms==0&&speed>=-.5f&&speed<=.5f;
}
/* Roots cover record/key/refund even after native dictionaries stop owning
 * them. A root allocation failure always precedes ownership/wallet mutation. */
static struct {
 DRoot world,vehicle,key,owned,refund,denominator,owned_dict,garage_dict,storage_dict;
} sale;
static void release_sale(void){
 ddrop(&sale.world);ddrop(&sale.vehicle);ddrop(&sale.key);ddrop(&sale.owned);ddrop(&sale.refund);ddrop(&sale.denominator);
 ddrop(&sale.owned_dict);ddrop(&sale.garage_dict);ddrop(&sale.storage_dict);sale_busy=0;
}
#include "context.h"
static int sale_remove(void *unused){
 (void)unused;
 PTR world=dget(sale.world),vehicle=dget(sale.vehicle),key_obj=dget(sale.key),owned=dget(sale.owned);
 /* Exit is a native mutation/callback boundary. Revalidate the exact instance,
  * passenger seats and emptiness after it, before the first economic effect. */
 if(!sale_context_current()||selected(world,vehicle,key_obj)!=owned||!empty_stopped(vehicle)||
    !snapshot_absent(dget(sale.storage_dict),key_obj))return 0;
 return remove_exact(dget(sale.owned_dict),key_obj,owned);
}
static int sale_credit(void *unused){
 (void)unused;int ok=1;PTR owned=0;
 if(!sale_context_current()||entry(dget(sale.owned_dict),dget(sale.key),&owned)!=0)return 0;
 PTR refund=bdata(dget(sale.refund));if(!refund)return 0;
 void *a[]={refund};
 PTR result=call(credit_m,dget(sale.world),a,&ok);
 int paid=0;
 if(!ok||!moon_sale_bool_data(bdata(result),&paid))return -1;
 return paid;
}
static int sale_restore(void *unused){
 (void)unused;PTR dictionary=dget(sale.owned_dict),key_obj=dget(sale.key),after=0;
 /* Do not overwrite a replacement entry installed by native callbacks. */
 if(!sale_context_current()||entry(dictionary,key_obj,&after)!=0)return -1;
 int ok=1;void *a[]={key_obj,dget(sale.owned)};
 call(get_method(object_class(dictionary),"set_Item",2),dictionary,a,&ok);
 return ok&&exact(dictionary,key_obj,dget(sale.owned))?1:-1;
}
static int sale_finish(void *unused){
 (void)unused;int ok=1;
 PTR world=dget(sale.world),vehicle=dget(sale.vehicle),key_obj=dget(sale.key);
 PTR owned=0;
 if(!sale_context_current()||entry(dget(sale.owned_dict),key_obj,&owned)!=0||
    !exact(dget(sale.garage_dict),key_obj,vehicle))return -1;
 if(!snapshot_absent(dget(sale.storage_dict),key_obj))return -1;
 call(prepare_m,vehicle,0,&ok);
 if(!ok||!snapshot_absent(dget(sale.storage_dict),key_obj))return -1;
 if(remove_exact(dget(sale.garage_dict),key_obj,vehicle)!=1)return -1;
 PTR go=call(object_m,vehicle,0,&ok);if(!ok||!go)return -1;
 void *destroy[]={go};call(destroy_m,0,destroy,&ok);if(!ok)return -1;
 int revision=bint(revision_m,world,&ok);if(!ok||revision==2147483647)return -1;
 revision++;void *a[]={&revision};call(set_revision_m,world,a,&ok);if(!ok)return -1;
 call(moved_m,world,0,&ok);if(!ok)return -1;
 call(save_m,world,0,&ok);return ok?1:-1;
}
static void sell(PTR world,PTR vehicle){
 int ok=1;PTR key_obj=dget(pending_key),owned=dget(pending_owned);
 if(sales_blocked||sale_busy){event("sales-blocked-uncertain-transaction");return;}
 PTR current_key=mf_vehicle_storage(vehicle,&ok);
 if(!ok||!mf_equal(current_key,key_obj)||selected(world,vehicle,key_obj)!=owned){event("sale-selection-changed-or-passengers");return;}
 if(!empty_stopped(vehicle)){event("empty-and-stop-required");return;}
 sale_busy=1;sale.world=dhold(world);sale.vehicle=dhold(vehicle);sale.key=dhold(key_obj);sale.owned=dhold(owned);
 if(!sale.world||!sale.vehicle||!sale.key||!sale.owned)goto failed;
 int index=bint(index_m,vehicle,&ok);void *ia[]={&index};PTR offer=call(offer_m,0,ia,&ok);
 if(!ok||!offer)goto failed;
 PTR price_f=get_field(object_class(offer),"price");if(!price_f)goto failed;
 unsigned char price[16];field_value(offer,price_f,price);
 int two=2;void *ta[]={&two};sale.denominator=dhold(call(decimal_two,0,ta,&ok));
 PTR denominator=bdata(dget(sale.denominator));
 if(!ok||!denominator)goto failed;
 void *da_args[]={price,denominator};sale.refund=dhold(call(decimal_div,0,da_args,&ok));
 if(!ok||!bdata(dget(sale.refund)))goto failed;
 PTR dictionary=0;field_value(world,owned_f,&dictionary);sale.owned_dict=dhold(dictionary);
 field_value(world,garage_f,&dictionary);sale.garage_dict=dhold(dictionary);
 field_value(world,storage_f,&dictionary);sale.storage_dict=dhold(dictionary);
 if(!sale.owned_dict||!sale.garage_dict||!sale.storage_dict)goto failed;
 if(!snapshot_absent(dget(sale.storage_dict),key_obj)){event("persisted-state-check-unavailable-sale-blocked");release_sale();return;}
 if(!exact(dget(sale.owned_dict),key_obj,owned)||!exact(dget(sale.garage_dict),key_obj,vehicle))goto failed;
 /* Exit may refuse unsafe positions. No ownership or wallet mutation yet. */
 if(!bbool(exit_m,vehicle,&ok)||!ok){event("exit-refused");release_sale();return;}
 static const MoonSaleOps ops={sale_remove,sale_credit,sale_restore,sale_finish};
 enum MoonSaleResult result=moon_sale_commit(&ops,0,&sales_blocked);
 static const char *const messages[]={"sales-blocked-uncertain-transaction","ownership-remove-refused","ownership-remove-uncertain-sales-blocked","payment-refused-restored","payment-refused-restore-uncertain-sales-blocked","payment-uncertain-sales-blocked","sale-committed-cleanup-uncertain-sales-blocked","vehicle-sold-half-price-save-requested"};
 event(messages[result]);release_sale();return;
failed:
 event("sale-preflight-failed");release_sale();
}
static int before(int id,Context *c){
 (void)id;if(!on_thread()||sale_busy)return 0;
 PTR walker=(PTR)c->rcx,local=0;int actor_ok=moon_local_actor_read(&local);
 /* Replacing the local walker cannot inherit confirmation or turn a held
  * chord into a new eligible edge. Known peer callbacks remain inert. */
 enum MoonSaleActor actor=moon_sale_actor(actor_ok,local,walker,dget(pending_walker),pending_walker!=0);
 if(actor==MOON_SALE_ACTOR_UNAVAILABLE){cancel();moon_input_action("sell-cars.sell","Alt+Delete",0);return 0;}
 if(actor==MOON_SALE_ACTOR_PEER)return 0;
 int ok=1;clock_now=bnumber(time_m,0,&ok);
 PTR world=0,vehicle=0;int ready=input_ready((PTR)c->rcx,&world,&vehicle,1);
 ready=ready&&ok&&clock_now>=0&&clock_now<1.0e30f&&!sales_blocked;
 int edge=moon_input_action("sell-cars.sell","Alt+Delete",ready).pressed;
 if(!ready||clock_now<armed_at||clock_now>=deadline||world!=dget(pending_world)||vehicle!=dget(pending_vehicle))cancel();
 if(ready&&edge){
  if(pending_vehicle){sell(world,vehicle);cancel();}
  else{
   float until=0;if(!vehicle_read_deadline(clock_now,5,&until)){cancel();event("sale-confirmation-clock-unavailable");return 0;}
   PTR key_obj=mf_vehicle_storage(vehicle,&ok);pending_key=dhold(key_obj);
   PTR owned=ok&&pending_key?selected(world,vehicle,dget(pending_key)):0;
   pending_owned=dhold(owned);pending_world=dhold(world);pending_vehicle=dhold(vehicle);pending_walker=dhold(walker);
   if(!dget(pending_key)||!dget(pending_owned)||!dget(pending_world)||!dget(pending_vehicle)||!dget(pending_walker)){cancel();event("owned-unoccupied-exact-car-required");}
   else{armed_at=clock_now;deadline=until;event("press-alt-delete-again-within-five-seconds-to-sell");}
  }
 }
 return 0;
}
int MoonLab_ModStart(PTR domain,PTR game){
 if(!base_bind(domain,game,"sell-cars.jsonl")||!mf_bind(game,1)||!moon_local_actor_bind())return 0;
 (void)pressed;
#define M(F,NS,T,N,S) F=dmethod(NS,T,N,S);if(!F){event(T "::" N " binding-failed");return 0;}
 M(index_m,"MoonCleaners","MoonPrototypeVehicle","get_Index","");M(offer_m,"MoonCleaners","MoonWorld","GarageOfferFor","System.Int32");M(decimal_div,"System","Decimal","op_Division","System.Decimal;System.Decimal");M(decimal_two,"System","Decimal","op_Implicit","System.Int32");M(credit_m,"MoonCleaners","MoonWorld","CreditShare","System.Decimal");M(exit_m,"MoonCleaners","MoonPrototypeVehicle","Exit","");M(prepare_m,"MoonCleaners","MoonPrototypeVehicle","PrepareRemoval","");M(object_m,"UnityEngine","Component","get_gameObject","");M(destroy_m,"UnityEngine","Object","Destroy","UnityEngine.Object");M(revision_m,"MoonCleaners","MoonWorld","get_OwnedVehiclesRevision","");M(set_revision_m,"MoonCleaners","MoonWorld","set_OwnedVehiclesRevision","System.Int32");M(moved_m,"MoonCleaners","MoonWorld","CrewVehiclesMoved","");M(save_m,"MoonCleaners","MoonWorld","RequestSave","");M(cargo_m,"MoonCleaners","MoonPrototypeVehicle","get_CargoCount","");M(dust_m,"MoonCleaners","MoonPrototypeVehicle","get_DustKilograms","");M(speed_m,"MoonCleaners","MoonPrototypeVehicle","get_Speed","");
#undef M
 PTR wc=find_class("MoonCleaners","MoonWorld");owned_f=get_field(wc,"ownedVehicles");garage_f=get_field(wc,"garageVehicles");storage_f=get_field(wc,"vehicleStorage");
 if(!owned_f||!garage_f||!storage_f||!prepare_hook(method("MoonCleaners","MoonWalker","Update",0),100671733,0,game))return 0;
 before_hook=before;return install_hooks();
}
