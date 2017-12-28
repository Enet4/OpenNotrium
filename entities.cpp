//entities, such as map, creatures, items

#include "entities.h"
#include "func.h"
#include <cstring>
#include <cmath>

using std::vector;

/// obtain reference to grid point at the given grid coordinates
map::grid_point& map::at(int x, int y) {
	return this->grid[y * this->sizex + x];
}
/// obtain reference to grid point at the given grid coordinates (const version)
const map::grid_point& map::at(int x, int y) const {
	return this->grid[y * this->sizex + x];
}

/// obtain reference to grid point at the given real coordinates
map::grid_point& map::at_real(float x, float y) {
	return this->at(static_cast<int>(x/grid_size), static_cast<int>(y/grid_size));
}

/// obtain reference to grid point at the given real coordinates (const version)
const map::grid_point& map::at_real(float x, float y) const {
	return this->at(static_cast<int>(x/grid_size), static_cast<int>(y/grid_size));
}

// base constructor
map::map(int sizex, int sizey, int climate)
:	sizex(sizex), sizey(sizey), climate_number(climate), area_type(-1), been_here_already(false),
	object(), creature(), lights(), items()
{
	//initialize grid
	this->grid = std::unique_ptr<grid_point[]>(new grid_point[sizex * sizey]);
}

//normal constructor
map::map(int sizex, int sizey, float amount_multiplier, int climate,
        vector<int> terrain_types, vector<bool> no_random_terrain_types,
        vector<bool> do_not_place_on_map_edges, vector<bool> terrain_is_hazardous,
        vector<int> prop_amounts, vector<int> prop_objects, vector<int> alien_types,
        vector<int> alien_amounts, vector<int> alien_sides, int items_amount)
:	map(sizex, sizey, climate)
{
	generate_map(amount_multiplier, terrain_types, no_random_terrain_types, do_not_place_on_map_edges,
			terrain_is_hazardous,prop_amounts,prop_objects, alien_types, alien_amounts, alien_sides);
}

// loading constructor
map::map(int sizex, int sizey, size_t creature_amount, size_t object_amount, size_t item_amount, int climate)
:	map(sizex, sizey, climate)
{
	this->creature.reserve(creature_amount);
	this->object.reserve(object_amount);
	this->items.reserve(item_amount);

	//initialize grid
	this->grid = std::unique_ptr<grid_point[]>(new grid_point[sizex * sizey]);
}

//find out on which map square is each creature
void map::check_creatures(void){

	//squares have no creatures by default
	for (int i=0; i<sizex; i++){
		for (int j=0; j<sizey; j++){
			at(i, j).total_creatures = 0;
		}
	}
	//find out the squares
	for (int k=0; k<creature.size(); k++){
		if(creature[k].dead)continue;
		auto& point = at_real(creature[k].x, creature[k].y);
		if(point.total_creatures>=20)continue;
		point.creatures[point.total_creatures]=k;
		point.total_creatures++;
	}

}


void map::generate_map(float amount_multiplier, const vector<int>& terrain_types,
                       const vector<bool>& no_random_terrain_types, const vector<bool>& do_not_place_on_map_edges,
                       const vector<bool>& terrain_is_hazardous, const vector<int>& prop_amounts,
                       const vector<int>& prop_objects, const vector<int>& alien_types,
                       const vector<int>& alien_amounts, const vector<int>& alien_sides){

	//special locations
	float player_start_x=randDouble(sizex*minimum_distance_from_edge*grid_size,sizex*grid_size-sizex*grid_size*minimum_distance_from_edge);
	float player_start_y=randDouble(sizey*minimum_distance_from_edge*grid_size,sizey*grid_size-sizey*grid_size*minimum_distance_from_edge);


	//zero the particles
	/*for (i=0; i<maximum_particles; i++){
		particles[i].dead=true;
	}*/

	//zero the bullets
	bullets.clear();
	/*for (i=0; i<maximum_bullets; i++){
		ZeroMemory(&bullets[i],sizeof(bullets[i]));
		bullets[i].dead=true;
	}*/

	//zero the lights
	lights.clear();
	/*for (i=0; i<maximum_lights; i++){
		ZeroMemory(&lights[i],sizeof(lights[i]));
		lights[i].dead=true;
	}*/

	/*//zero the items
	for (i=0; i<total_items; i++){
		ZeroMemory(&items[i],sizeof(items[i]));
		items[i].dead=true;
	}*/

	//object generation algorithm
	/*for (i=0; i<total_objects; i++){
		object[i].dead=false;
	}*/

	//ground generation algorithm
	//all starts with the first texture
	for (int i=0; i<sizex; i++){
		for (int j=0; j<sizey; j++){
			if(terrain_types.size()>0)
				at(i, j).terrain_type=terrain_types[0];
			else
				at(i,j).terrain_type=2;
			/*
			grid[i].grid[j].light_rgb[0]=0;
			grid[i].grid[j].light_rgb[0]=1;
			grid[i].grid[j].light_rgb[0]=2;
			*/
		}
	}

	//throw in some other surface
	if(terrain_types.size()>0)
	for (int k=0; k<sizex*3; k++){//map size tells how many spots there are
		int ground_type=terrain_types[randInt(0,terrain_types.size())];
		int target_x=randInt(0,sizex);
		int target_y=randInt(0,sizey);
		int size=randInt(3,6);
		int start_x=target_x-size;
		int start_y=target_y-size;
		int end_x=target_x+size;
		int end_y=target_y+size;
		if(start_x<0)start_x=0;
		if(start_y<0)start_y=0;
		if(end_x>sizex-1)end_x=sizex-1;
		if(end_y>sizey-1)end_y=sizey-1;

		for (int i=start_x; i<end_x; i++){
			for (int j=start_y; j<end_y; j++){
				if(sqrt((float)(sqr(i-target_x)+sqr(j-target_y)))<size){
					//do not place this type on edges
					if(do_not_place_on_map_edges[ground_type]){
						if((i<2)||(j<2)||(i>sizex-4)||(j>sizey-4))
							continue;
					}
					at(i,j).terrain_type=ground_type;
				}
			}
		}

	}

	//throw in props
	//int prop_counter=0;
	for (int j=0; j<prop_amounts.size(); j++){
		for (int i=0; i<(int)(prop_amounts[j]*amount_multiplier); i++){
			map_object temp_object;
			int place_bad=100;
			while(place_bad>0){

				temp_object.x=randDouble(0,(sizex-2)*(grid_size));
				temp_object.y=randDouble(0,(sizey-2)*(grid_size));
        auto grid_x = static_cast<int>(temp_object.x / grid_size);
        auto grid_y = static_cast<int>(temp_object.y / grid_size);
				//see if it's on some hazardous terrain type
				if(terrain_is_hazardous[grid_x*sizey + grid_y] != 0)
					place_bad--;
				else if(no_random_terrain_types[at(grid_x, grid_y).terrain_type])
					place_bad--;
				else
					place_bad=0;
			}
			temp_object.dead=false;
			temp_object.rotation=randDouble(0,2*pi);
			temp_object.type=prop_objects[j];
			temp_object.size=1;
			temp_object.sway_phase=0;
			temp_object.sway_power=0;

			//prop_counter++;
			object.push_back(temp_object);
		}
	}

	creature_base temp_creature;
	creature.push_back(temp_creature);

	//throw in the creatures
	//float diameter=sqrtf(sqr(sizex*grid_size)+sqr(sizey*grid_size));
	//for (i=0; i<total_creatures; i++){

	int creature_counter=1;//0=player

	for (int a=0; a<alien_amounts.size(); a++){
		for (int b=0; b<(int)(alien_amounts[a]); b++){
			bool place_not_ok=true;
			while(place_not_ok){

				//pick a random grid point
				int x=randInt(2,sizex-2);
				int y=randInt(2,sizey-2);

				//bool aa=terrain_is_hazardous[aaa];

				if(terrain_is_hazardous[(int)(x)*sizey+(int)(y)])
					continue;
				if(no_random_terrain_types[at(x, y).terrain_type])
					continue;

				//randomly the creature can be placed here or not
				/*float distance=sqrtf(sqr(x*grid_size-player_start_x)+sqr(y*grid_size-player_start_y));
				//if(distance<4000)continue;
				if(randInt(0,(int)(diameter/distance))!=0)continue;*/

				int i=creature_counter;
				creature_counter++;

				creature_base temp_creature;
				creature.push_back(temp_creature);

				memset(&creature[i], 0, sizeof(creature[i]));
				creature[i].dead=false;
				creature[i].rotation=randDouble(0,2*pi);
				creature[i].rotation_head=creature[i].rotation;
				creature[i].rotation_legs=creature[i].rotation;
				creature[i].size=randDouble(0.9f,1.0f);
				creature[i].type=alien_types[a];
				//creature[i].health=randDouble(0.8f,1.2f);
				creature[i].killed=false;
				creature[i].side=alien_sides[a];
				//creature[i].sneak=false;
				creature[i].animation[0]=0;
				creature[i].animation[1]=2;
				creature[i].animation[2]=5;
				//creature[i].may_change_area=true;
				creature[i].weapon_effects_amount=0;
				creature[i].carry_light=-1;
				creature[i].dialog=-1;
				creature[i].fire_timer=0;
				creature[i].last_bullet_hit_from_creature_number=-1;
				creature[i].carried_creature=-1;
				creature[i].vanish_timer=0;
				creature[i].wall_between_creature_and_player=-1;
				creature[i].force_AI=false;
				creature[i].anger_level=0;
				creature[i].touched_enemy=-1;


				creature[i].x=x*grid_size+randDouble(0,grid_size);
				creature[i].y=y*grid_size+randDouble(0,grid_size);
				creature[i].x2=x*grid_size+randDouble(0,grid_size);
				creature[i].y2=y*grid_size+randDouble(0,grid_size);
				creature[i].move_to_x=creature[i].x;
				creature[i].move_to_y=creature[i].y;
				place_not_ok=false;
			}
		}
	}


	//player
	auto& player = creature[0];
	memset(&player, 0, sizeof(player));
	player.dead=false;
	player.rotation=randDouble(0,2*pi);
	player.rotation_head=player.rotation;
	player.rotation_legs=player.rotation;
	player.size=1;
	player.type=0;
	//player.health=1;
	player.killed=false;
	player.side=0;
	//player.sneak=false;
	//player.may_change_area=true;
	player.animation[0]=0;
	player.animation[1]=2;
	player.animation[2]=5;
	player.x=player_start_x;
	player.y=player_start_y;
	player.x2=player.x;
	player.y2=player.y;
	player.carry_light=-1;
	player.dialog=-1;
	player.fire_timer=0;
	player.last_bullet_hit_from_creature_number=-1;
	player.carried_creature=-1;
	player.vanish_timer=0;
	player.touched_enemy=-1;

}

//initializes the map items
void map::initialize_items(void){
	for (int i=0; i<sizex; i++){
		for (int j=0; j<sizey; j++){
			at(i,j).items.clear();
			//for (k=0; k<maximum_objects_on_grid; k++){
			//	grid[i].grid[j].objects[k]=
			//}
		}
	}
	for (unsigned int k=0; k<items.size(); k++){
		if(items[k].dead)continue;
		//if(grid[x].grid[y].total_objects<maximum_objects_on_grid-1){
  	at_real(items[k].x, items[k].y).items.push_back(k);
		//	grid[x].grid[y].total_objects++;
		//}
	}

	/*//find out on which map square is each object
	int items_here;
	int items_list[100];
	for (i=0; i<sizex; i++){
		for (j=0; j<sizey; j++){
			items_here=0;

			for (k=0; k<items.size(); k++){
				if(items[k].dead)continue;
				//if the object is on this grid square
				if((items[k].x>=i*grid_size)
				&&(items[k].x<(i+1)*grid_size)
				&&(items[k].y>=j*grid_size)
				&&(items[k].y<(j+1)*grid_size)){
					items_list[items_here]=k;
					items_here++;
				}
			}
			//found objects
			SAFE_DELETE_ARRAY(grid[i].grid[j].items);
			grid[i].grid[j].items=NULL;
			grid[i].grid[j].total_items=items_here;
			if(items_here>0)
			{
				grid[i].grid[j].items = new int[items_here];
				for (k=0; k<items_here; k++){
					grid[i].grid[j].items[k]=items_list[k];
				}
			}
		}
	}*/
}


//initializes the map objects
void map::initialize_objects(void){
	//find out on which map square is each object
	/*int objects_here;
	int objects[100];
	for (i=0; i<sizex; i++){
		for (j=0; j<sizey; j++){
			objects_here=0;
			for (k=0; k<total_objects; k++){
				if(object[k].dead)continue;
				//if the object is on this grid square
				if((object[k].x>=i*grid_size)
				&&(object[k].x<(i+1)*grid_size)
				&&(object[k].y>=j*grid_size)
				&&(object[k].y<(j+1)*grid_size)){
					objects[objects_here]=k;
					objects_here++;
				}
			}
			//found objects
			SAFE_DELETE_ARRAY(grid[i].grid[j].objects);
			grid[i].grid[j].total_objects=objects_here;
			grid[i].grid[j].objects=NULL;
			if(objects_here>0)
			{
				grid[i].grid[j].objects = new int[objects_here];
				for (k=0; k<objects_here; k++){
					grid[i].grid[j].objects[k]=objects[k];
				}
			}
		}
	}*/

	for (int i=0; i<sizex; i++){
		for (int j=0; j<sizey; j++){
			at(i,j).objects.clear();
			//for (k=0; k<maximum_objects_on_grid; k++){
			//	grid[i].grid[j].objects[k]=
			//}
		}
	}
	for (unsigned int k=0; k<object.size(); k++){
		if(object[k].dead)continue;
		//if(grid[x].grid[y].total_objects<maximum_objects_on_grid-1){
		at_real(object[k].x, object[k].y).objects.push_back(k);
			//grid[x].grid[y].total_objects++;
		//}
	}
	/*		//found objects
			SAFE_DELETE_ARRAY(grid[i].grid[j].objects);
			grid[i].grid[j].total_objects=objects_here;
			grid[i].grid[j].objects=NULL;
			if(objects_here>0)
			{
				grid[i].grid[j].objects = new int[objects_here];
				for (k=0; k<objects_here; k++){
					grid[i].grid[j].objects[k]=objects[k];
				}
			}
		}
	}*/
}


int map::create_light(float x, float y, int type, float size, float r, float g, float b, float a, float time){


	light temp_light;
	memset(&temp_light, 0, sizeof(temp_light));

	temp_light.dead=false;
	temp_light.time=time;
	temp_light.r=r;
	temp_light.g=g;
	temp_light.b=b;
	temp_light.rotation=randDouble(0,2*pi);
	temp_light.size=size;
	temp_light.transparency=a;
	temp_light.type=type;
	temp_light.x=x-size*128*0.5f;
	temp_light.y=y-size*128*0.5f;
	temp_light.x2=x;
	temp_light.y2=y;
	temp_light.pulse_phase=randDouble(0,100);
	temp_light.pulse=1;

	lights.push_back(temp_light);

	return lights.size()-1;
}

void creature_base::die(void){
	animation[0]=-1;
	animation[2]=-1;

	animation[1]=10;
	animation_timer[1]=100;

	killed=true;
	//dead=true;
}


map_object::map_object(){
	x=0;
	y=0;
	rotation=0;
	size=1;
	dead=true;
	type=0;
	visible=true;

}


map_object::~map_object(){
}

creature_base::creature_base(void)
: left(0)
, right(0)
, up(0)
, down(0)
, fire(false)
, backward_forward_speed(0)
, turn_speed(0)
{}

creature_base::~creature_base(){
}


light::light(){
	x=0;
	y=0;
	rotation=0;
	size=1;
	dead=true;
	type=0;
	r=1;
	g=1;
	b=1;
	pulse=1;

}

light::~light(){

}



bullet::bullet(){
	dead=true;
}

item::item():map_object(){
	item_type=0;
	event_used=false;
	interval_timer=0;
	sound_timer=0;

}
