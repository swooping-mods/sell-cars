/* M18: remove one owned empty car and credit half its vanilla offer price. */
#include "base.h"
#include "../../runtime/native/input.h"
#include "../vehicle-framework/sdk/fleet_adapter.h"
static PTR index_m,offer_m,decimal_div,decimal_two,credit_m,exit_m,prepare_m,object_m,destroy_m,revision_m,set_revision_m,moved_m,save_m,cargo_m,dust_m,speed_m;
static PTR owned_f,garage_f,storage_f;static DRoot pending_world,pending_vehicle;static float deadline;
static void cancel(void){ddrop(&pending_world);ddrop(&pending_vehicle);deadline=0;}
static int remove_key(PTR dictionary,PTR key_obj,int *ok){if(!dictionary){*ok=0;return 0;}void *a[]={key_obj};PTR result=call(get_method(object_class(dictionary),"Remove",1),dictionary,a,ok);return result&&*ok&&*(unsigned char*)unbox(result);}
static void sell(PTR world,PTR vehicle){
 int ok=1;PTR key_obj=mf_vehicle_storage(vehicle,&ok),player=call(mf.local,0,0,&ok);
 PTR owned=mf_lookup(world,key_obj,&ok);MoonFleetRecord record={0};
 if(!ok||!owned||!key_obj||!player||!mf_read(owned,&record)||!mf_equal(record.storage,key_obj)){event("not-owned");return;}
 if(!moon_fleet_local(&mf_queries,&record,player)){event("not-local-owner");return;}
 int cargo=integer(cargo_m,vehicle,&ok);PTR dust=call(dust_m,vehicle,0,&ok);float speed=dnumber(speed_m,vehicle,&ok);
 if(!ok||!dust||cargo||*(double*)unbox(dust)>0.0001||speed>0.5f||speed< -0.5f){event("empty-and-stop-required");return;}
 int index=integer(index_m,vehicle,&ok);void *ia[]={&index};PTR offer=call(offer_m,0,ia,&ok);if(!ok||!offer){event("no-offer-price");return;}
 PTR price_f=get_field(object_class(offer),"price");if(!price_f)return;unsigned char price[16];field_value(offer,price_f,price);
 int two=2;void *ta[]={&two};PTR denominator=call(decimal_two,0,ta,&ok);if(!denominator||!ok)return;void *da_args[]={price,unbox(denominator)};PTR refund=call(decimal_div,0,da_args,&ok);if(!refund||!ok)return;
 PTR owned_dict=0,garage_dict=0,storage_dict=0;field_value(world,owned_f,&owned_dict);field_value(world,garage_f,&garage_dict);field_value(world,storage_f,&storage_dict);if(!owned_dict||!garage_dict||!storage_dict)return;
 /* Exit can refuse unsafe positions. Do not delete a car with its player seated. */
 if(!boolean(exit_m,vehicle,&ok)||!ok){event("exit-refused");return;}
 if(!remove_key(owned_dict,key_obj,&ok)){event("ownership-remove-failed");return;}
 void *ra[]={unbox(refund)};PTR paid=call(credit_m,world,ra,&ok);
 if(!ok||!paid||!*(unsigned char*)unbox(paid)){
  int restored=1;void *restore[]={key_obj,owned};call(get_method(object_class(owned_dict),"set_Item",2),owned_dict,restore,&restored);event(restored?"payment-refused-restored":"payment-refused-restore-failed");return;
 }
 call(prepare_m,vehicle,0,&ok);remove_key(garage_dict,key_obj,&ok);
 /* Empty storage has no transferable contents. Remove its old reusable key. */
 remove_key(storage_dict,key_obj,&ok);
 PTR go=call(object_m,vehicle,0,&ok);if(go){void *destroy[]={go};call(destroy_m,0,destroy,&ok);}
 int revision=integer(revision_m,world,&ok)+1;void *revision_args[]={&revision};call(set_revision_m,world,revision_args,&ok);call(moved_m,world,0,&ok);call(save_m,world,0,&ok);event(ok?"vehicle-sold-half-price":"sale-committed-cleanup-error");
}
static int before(int id,Context *c){
 (void)id;if(!on_thread())return 0;int ok=1;clock_now=dnumber(time_m,0,&ok);PTR world=0,vehicle=0;int ready=input_ready((PTR)c->rcx,&world,&vehicle,1),edge=moon_input_action("sell-cars.sell","Alt+Delete",ready).pressed;
 if(!ready||clock_now>deadline||world!=dget(pending_world)||vehicle!=dget(pending_vehicle))cancel();
 if(ready&&edge){if(pending_vehicle){sell(world,vehicle);cancel();}else{pending_world=dhold(world);pending_vehicle=dhold(vehicle);deadline=clock_now+5;event("press-alt-delete-again-within-five-seconds-to-sell");}}
 return 0;
}
int MoonLab_ModStart(PTR domain,PTR game){
 if(!base_bind(domain,game,"sell-cars.jsonl")||!mf_bind(game,0))return 0;
(void)pressed;
#define M(F,NS,T,N,S) F=dmethod(NS,T,N,S);if(!F){event(T "::" N " binding-failed");return 0;}
 M(index_m,"MoonCleaners","MoonPrototypeVehicle","get_Index","");M(offer_m,"MoonCleaners","MoonWorld","GarageOfferFor","System.Int32");M(decimal_div,"System","Decimal","op_Division","System.Decimal;System.Decimal");M(decimal_two,"System","Decimal","op_Implicit","System.Int32");M(credit_m,"MoonCleaners","MoonWorld","CreditShare","System.Decimal");M(exit_m,"MoonCleaners","MoonPrototypeVehicle","Exit","");M(prepare_m,"MoonCleaners","MoonPrototypeVehicle","PrepareRemoval","");M(object_m,"UnityEngine","Component","get_gameObject","");M(destroy_m,"UnityEngine","Object","Destroy","UnityEngine.Object");M(revision_m,"MoonCleaners","MoonWorld","get_OwnedVehiclesRevision","");M(set_revision_m,"MoonCleaners","MoonWorld","set_OwnedVehiclesRevision","System.Int32");M(moved_m,"MoonCleaners","MoonWorld","CrewVehiclesMoved","");M(save_m,"MoonCleaners","MoonWorld","RequestSave","");M(cargo_m,"MoonCleaners","MoonPrototypeVehicle","get_CargoCount","");M(dust_m,"MoonCleaners","MoonPrototypeVehicle","get_DustKilograms","");M(speed_m,"MoonCleaners","MoonPrototypeVehicle","get_Speed","");
#undef M
 PTR wc=find_class("MoonCleaners","MoonWorld");owned_f=get_field(wc,"ownedVehicles");garage_f=get_field(wc,"garageVehicles");storage_f=get_field(wc,"vehicleStorage");
 if(!owned_f||!garage_f||!storage_f||!prepare_hook(method("MoonCleaners","MoonWalker","Update",0),100671733,0,game))return 0;before_hook=before;return install_hooks();
}
