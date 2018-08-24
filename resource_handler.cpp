#include "resource_handler.h"
#include <iostream>
#include <SDL.h>
#include <SDL_image.h>

using std::cerr;
using std::string;
using std::vector;
using namespace Debugger;

int resource_handler::load_texture(const string& name, const string& mod_name){//loads the texture if it's unique

    if(name=="none")
        return -1;

    //find if we're already recorded this texture name
    for(auto i=0u;i<textures.size();i++){
        if((textures[i].texture_name==name)&&(textures[i].mod_name==mod_name))
            return i;
    }


    std::string tempstring="Load Texture ";
    tempstring+=name;
    debug->debug_output(tempstring,Action::START,Logfile::STARTUP);

    //create new texture loading primitive
    texture_handling_primitive_base temp_load;
    temp_load.last_used=system_time;
    temp_load.mod_name=mod_name;
    temp_load.texture_handle_in_grim=-1;
    temp_load.texture_name=name;

    //check if such a file exists
    auto filepath = "textures/" + mod_name + "/" + name;
    bool OK = grim->File_Exists(filepath);

    //try default directory
    if(!OK){
        filepath = "textures/" + name;
        OK=grim->File_Exists(filepath);
    }
    if(!OK){
        debug->debug_output("Loading Texture!", Action::FAIL_AND_END, Logfile::STARTUP);
    }

    debug->debug_output(tempstring, Action::END, Logfile::STARTUP);

    textures.push_back(temp_load);
    return textures.size()-1;
}


int resource_handler::load_sample(const string& name, int samples, const string& mod_name){//loads the sample if it's unique
    //TODO: use samples parameter if it is important

    if(name=="none.wav")return -1;
    if(name=="none")return -1;


    if(!play_sound)return -1;
    if(!sound_initialized)return -1;
    if(samples_loaded>=maximum_samples)return -1;

    int i;
    //find if the sample was already loaded
    for(i=0;i<samples_loaded;i++){
        //if(strcmpi(name,sample_name[i])==0){return i;}
        if((sample_name[i]==name)&&(sample_name_mod[i]==mod_name)){return i;}
    }

    //not loaded, load it
    {

        std::string tempstring="Load Sample ";
        tempstring+=name;
        debug->debug_output(tempstring,Action::START,Logfile::STARTUP);

        if(samples==-1)samples=2;

        char temprivi[300];
        bool res;
        //try mod directory
        strcpy(temprivi,"sound/");
        strcat(temprivi,mod_name.c_str());
        strcat(temprivi,"/");
        strcat(temprivi,name.c_str());
        res=g_pSoundManager->Create(sample[samples_loaded], temprivi );

        //try default directory
        if(!res){
            SAFE_DELETE(sample[samples_loaded]);
            strcpy(temprivi,"sound/");
            strcat(temprivi,name.c_str());
            res = g_pSoundManager->Create(sample[samples_loaded], temprivi );
        }
        if (res) {
            //store name
            sample_name[samples_loaded]=name;
            sample_name_mod[samples_loaded]=mod_name;

            debug->debug_output(tempstring,Action::END, Logfile::STARTUP);

            samples_loaded++;
            return samples_loaded-1;
        }

        debug->debug_output("Loading Sample",Action::FAIL_AND_END,Logfile::STARTUP);
        return -2;
    }

}

void resource_handler::initialize_resource_handler(Engine *grim_engine, debugger *debugger, SoundManager* g_pSoundManager, bool play_sound, bool sound_initialized){
    debug=debugger;
    grim=grim_engine;
    this->g_pSoundManager=g_pSoundManager;
    this->play_sound=play_sound;
    this->sound_initialized=sound_initialized;
}

void resource_handler::uninitialize(void){
    for(int a=0;a<maximum_samples;a++){
        SAFE_DELETE(sample[a]);
    }
}

resource_handler::resource_handler(void)
:   debug(nullptr), grim(nullptr), g_pSoundManager(nullptr), textures(), textures_count(0), sample(), samples_loaded(0),
    system_time(0), high_texture_count(0), sound_initialized(false),
    play_sound(false)
{
    //clear all samples
    for(auto& s : sample){
        s = nullptr;
    }
}

void resource_handler::Texture_Set(int number){
    if (number == -1) {
        // deliberate `none` texture, don't log
        return;
    }
    if (number >= textures.size()) {
        cerr << "Invalid texture number " << number << ", ignoring\n";
        return;
    }

    auto& tex = this->textures[number];

    //first make sure the texture is loaded
    if(tex.texture_handle_in_grim<0){
        auto tex_id = load_texture_in_grim(
            tex.texture_name.c_str(),
            tex.mod_name);
        if (tex_id == -1) {
            cerr << "Failed to set texture \"" << tex.texture_name << "\"\n";
            return;
        }
        tex.texture_handle_in_grim = tex_id;
    }

    tex.last_used=system_time;
    grim->Texture_Set(tex.texture_handle_in_grim);
}

int resource_handler::load_texture_in_grim(const char *name, const string& mod_name){
    char temprivi[1000];
    //try mod directory
    strcpy(temprivi,"textures/");
    strcat(temprivi,mod_name.c_str());
    strcat(temprivi,"/");
    strcat(temprivi,name);
    int err = grim->Texture_Load(temprivi, temprivi);

    //try default directory
    if(err) {
        cerr << "Texture load failed: " << IMG_GetError() << "; Attempting default folder\n";
        strcpy(temprivi,"textures/");
        strcat(temprivi, name);

        err = grim->Texture_Load(temprivi, temprivi);
        if (err) {
            if (err == -1) {
                cerr << "Texture load for image " << name << " in mod " << mod_name << " failed: " << IMG_GetError() << "\n";
            } else if (err == -2) {
                cerr << "Texture load for image " << name << " in mod " << mod_name << " failed: bad format\n";
            }
            return -1;
        }
    }
    // try placeholder texture
    if (err) {
        debug->debug_output("Failed to load texture, using placeholder instead", Action::LOG, Logfile::STARTUP);
        strcpy(temprivi, "textures/placeholder.png");
        err = grim->Texture_Load(temprivi, temprivi);
    }

    int texture_number=grim->Texture_Get(temprivi);

    if(texture_number == -1) {
        //we're still unable to load the texture, all slots must be full
        //release some slots and try again
        unload_unneeded_textures(false);
        texture_number=grim->Texture_Get(temprivi);
        if (texture_number == -1) {
            cerr << "Failed to get texture.\n";
        }
    }

    if (texture_number > high_texture_count) {
        high_texture_count = texture_number;
    }

    return texture_number;
}

void resource_handler::unload_unneeded_textures(bool unload_all){
    for(auto& tex : textures) {
        if(tex.texture_handle_in_grim >= 0) {
            if((system_time - tex.last_used > 300) || unload_all) {
                grim->Texture_Delete(tex.texture_handle_in_grim);
                tex.texture_handle_in_grim = -1;
            }
        }
    }
}
