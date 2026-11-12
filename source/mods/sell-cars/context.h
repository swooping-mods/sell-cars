/* Native Exit/provider callbacks can change the walker's world while keeping
 * its identity and the old rooted world alive. Every transaction phase checks
 * this exact current world before touching the captured dictionaries. */
static int sale_context_current(void){
 PTR world=dget(sale.world),value=0,local=0,current_world=0;
 int ok=1;
 if(!moon_local_actor_read(&local)||!local||local!=dget(pending_walker))return 0;
 field_value(local,world_f,&current_world);if(current_world!=world)return 0;
 if(!alive(world)||!alive(dget(sale.vehicle))||!host()||bbool(restoring_m,world,&ok)||!ok)return 0;
 field_value(world,owned_f,&value);if(value!=dget(sale.owned_dict))return 0;
 field_value(world,garage_f,&value);if(value!=dget(sale.garage_dict))return 0;
 field_value(world,storage_f,&value);return value==dget(sale.storage_dict);
}
