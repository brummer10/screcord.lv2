/*
 * Copyright (C) 2013 Hermann Meyer
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 * --------------------------------------------------------------------------
 */

#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/stat.h>

#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <climits>
#include <cstring>

#include <sndfile.hh>

#include "sc_record.h"

////////////////////////////// LOCAL INCLUDES //////////////////////////

#include "screcord1.cc"

////////////////////////////// PLUG-IN CLASS ///////////////////////////

class SCrecord
{
private:
  LV2_URID_Map*        map;
  int32_t     rt_prio;
  int32_t     rt_policy;
  // pointer to buffer
  float*      output;
  float*      input;
  float*      output1;
  float*      input1;
  // pointer to dsp class
  screcord::SCapture*  record;
  // private functions
  inline void run_dsp_(uint32_t n_samples);
  inline void run_dsp_st(uint32_t n_samples);
  inline void connect_(uint32_t port,void* data);
  inline void init_dsp_(uint32_t rate);
  inline void init_dsp_st(uint32_t rate);
  inline void connect_all__ports(uint32_t port, void* data);
  inline void activate_f();
  inline void clean_up();
  inline void deactivate_f();

public:
  // LV2 Descriptor
  static const LV2_Descriptor descriptor;
  static const LV2_Descriptor descriptor1;
  LV2_State_Make_Path* make_path;
  // static wrapper to private functions
  static void deactivate(LV2_Handle instance);
  static void cleanup(LV2_Handle instance);
  static void run(LV2_Handle instance, uint32_t n_samples);
  static void run_st(LV2_Handle instance, uint32_t n_samples);
  static void activate(LV2_Handle instance);
  static void connect_port(LV2_Handle instance, uint32_t port, void* data);
  static LV2_Handle instantiate(const LV2_Descriptor* descriptor,
                                double rate, const char* bundle_path,
                                const LV2_Feature* const* features);
  static LV2_Handle instantiate_st(const LV2_Descriptor* descriptor,
                                double rate, const char* bundle_path,
                                const LV2_Feature* const* features);
  SCrecord();
  ~SCrecord();
};

// constructor
SCrecord::SCrecord() :
  rt_prio(0),
  rt_policy(0),
  output(NULL),
  input(NULL),
  output1(NULL),
  input1(NULL),
  make_path(NULL) { };

// destructor
SCrecord::~SCrecord()
{
  record->activate_plugin(false, record);
  record->delete_instance(record);
};

///////////////////////// PRIVATE CLASS  FUNCTIONS /////////////////////

void SCrecord::init_dsp_(uint32_t rate)
{
  record = new screcord::SCapture(1);
  record->set_samplerate(rate, record); // init the DSP class
  record->make_path = make_path;
  record->set_thread_prio(rt_prio, rt_policy);
}

void SCrecord::init_dsp_st(uint32_t rate)
{
  record = new screcord::SCapture(2);
  record->set_samplerate(rate, record); // init the DSP class
  record->make_path = make_path;
  record->set_thread_prio(rt_prio, rt_policy);
}

// connect the Ports used by the plug-in class
void SCrecord::connect_(uint32_t port,void* data)
{
  switch ((PortIndex)port)
    {
    case EFFECTS_OUTPUT:
      output = static_cast<float*>(data);
      break;
    case EFFECTS_INPUT:
      input = static_cast<float*>(data);
      break;
    case EFFECTS_OUTPUT1:
      output1 = static_cast<float*>(data);
      break;
    case EFFECTS_INPUT1:
      input1 = static_cast<float*>(data);
      break;
    default:
      break;
    }
}

void SCrecord::activate_f()
{
  // allocate the internal DSP mem
  record->activate_plugin(true, record);
}

void SCrecord::clean_up()
{
  // delete the internal DSP mem
  record->activate_plugin(false, record);
}

void SCrecord::deactivate_f()
{
  // delete the internal DSP mem
  record->activate_plugin(false, record);
}

void SCrecord::run_dsp_(uint32_t n_samples)
{
  record->mono_audio(static_cast<int>(n_samples), input, output, record);
}

void SCrecord::run_dsp_st(uint32_t n_samples)
{
  record->stereo_audio(static_cast<int>(n_samples), input, input1, output, output1, record);
}

void SCrecord::connect_all__ports(uint32_t port, void* data)
{
  // connect the Ports used by the plug-in class
  connect_(port,data); 
  // connect the Ports used by the DSP class
  record->connect_ports(port,  data, record);
}

////////////////////// STATIC CLASS  FUNCTIONS  ////////////////////////

LV2_Handle 
SCrecord::instantiate(const LV2_Descriptor* descriptor,
                            double rate, const char* bundle_path,
                            const LV2_Feature* const* features)
{
  // init the plug-in class
  SCrecord *self = new SCrecord();
  if (!self)
    {
      return NULL;
    }
  
  const LV2_Options_Option* options  = NULL;

  for (int32_t i = 0; features[i]; ++i)
    {
      if (!strcmp(features[i]->URI, LV2_URID__map))
        {
          self->map = (LV2_URID_Map*)features[i]->data;
        } 
      else if (!strcmp (features[i]->URI, LV2_OPTIONS__options))
        {
          options = (const LV2_Options_Option*)features[i]->data;
        }
      else if (!strcmp(features[i]->URI, LV2_STATE__makePath))
        {
          self->make_path = (LV2_State_Make_Path*)features[i]->data;
        }
    }
  if (self->map)
    {
      if (options)
        {
          LV2_URID atom_Int  = self->map->map (self->map->handle, LV2_ATOM__Int);
          LV2_URID tshed_pol = self->map->map (self->map->handle, "http://ardour.org/lv2/threads/#schedPolicy");
          LV2_URID tshed_pri = self->map->map (self->map->handle, "http://ardour.org/lv2/threads/#schedPriority");
          for (const LV2_Options_Option* o = options; o->key; ++o)
            {
            if (o->context == LV2_OPTIONS_INSTANCE &&
                o->key == tshed_pol &&
                o->type == atom_Int)
              {
                self->rt_policy = *(const int32_t*)o->value;
              }
            if (o->context == LV2_OPTIONS_INSTANCE &&
                o->key == tshed_pri &&
                o->type == atom_Int)
              {
                self->rt_prio = *(const int32_t*)o->value;
              }
           }
        }
    }

#ifndef  __MOD_DEVICES__
  if (!self->make_path)
    {
      fprintf(stderr, "Missing feature LV2_URID__makePath.\n");
    }
#endif
  self->init_dsp_((uint32_t)rate);

  return (LV2_Handle)self;
}

LV2_Handle 
SCrecord::instantiate_st(const LV2_Descriptor* descriptor,
                            double rate, const char* bundle_path,
                            const LV2_Feature* const* features)
{
  // init the plug-in class
  SCrecord *self = new SCrecord();
  if (!self)
    {
      return NULL;
    }
  
  const LV2_Options_Option* options  = NULL;

  for (int32_t i = 0; features[i]; ++i)
    {
      if (!strcmp(features[i]->URI, LV2_URID__map))
        {
          self->map = (LV2_URID_Map*)features[i]->data;
        } 
      else if (!strcmp (features[i]->URI, LV2_OPTIONS__options))
        {
          options = (const LV2_Options_Option*)features[i]->data;
        }
      else if (!strcmp(features[i]->URI, LV2_STATE__makePath))
        {
          self->make_path = (LV2_State_Make_Path*)features[i]->data;
        }
    }
  if (self->map)
    {
      if (options)
        {
          LV2_URID atom_Int  = self->map->map (self->map->handle, LV2_ATOM__Int);
          LV2_URID tshed_pol = self->map->map (self->map->handle, "http://ardour.org/lv2/threads/#schedPolicy");
          LV2_URID tshed_pri = self->map->map (self->map->handle, "http://ardour.org/lv2/threads/#schedPriority");
          for (const LV2_Options_Option* o = options; o->key; ++o)
            {
            if (o->context == LV2_OPTIONS_INSTANCE &&
                o->key == tshed_pol &&
                o->type == atom_Int)
              {
                self->rt_policy = *(const int32_t*)o->value;
              }
            if (o->context == LV2_OPTIONS_INSTANCE &&
                o->key == tshed_pri &&
                o->type == atom_Int)
              {
                self->rt_prio = *(const int32_t*)o->value;
              }
           }
        }
    }

#ifndef  __MOD_DEVICES__
  if (!self->make_path)
    {
      fprintf(stderr, "Missing feature LV2_URID__makePath.\n");
    }
#endif
  self->init_dsp_st((uint32_t)rate);

  return (LV2_Handle)self;
}

void SCrecord::connect_port(LV2_Handle instance, 
                                   uint32_t port, void* data)
{
  // connect all ports
  static_cast<SCrecord*>(instance)->connect_all__ports(port, data);
}

void SCrecord::activate(LV2_Handle instance)
{
  // allocate needed mem
  static_cast<SCrecord*>(instance)->activate_f();
}

void SCrecord::run(LV2_Handle instance, uint32_t n_samples)
{
  // run dsp
  static_cast<SCrecord*>(instance)->run_dsp_(n_samples);
}

void SCrecord::run_st(LV2_Handle instance, uint32_t n_samples)
{
  // run dsp
  static_cast<SCrecord*>(instance)->run_dsp_st(n_samples);
}

void SCrecord::deactivate(LV2_Handle instance)
{
  // free allocated mem
  static_cast<SCrecord*>(instance)->deactivate_f();
}

void SCrecord::cleanup(LV2_Handle instance)
{
  // well, clean up after us
  SCrecord* self = static_cast<SCrecord*>(instance);
  self->clean_up();
  delete self;
}

const LV2_Descriptor SCrecord::descriptor =
{
  SCPLUGIN_URI "#mono_record",
  SCrecord::instantiate,
  SCrecord::connect_port,
  SCrecord::activate,
  SCrecord::run,
  SCrecord::deactivate,
  SCrecord::cleanup,
  NULL
};

const LV2_Descriptor SCrecord::descriptor1 =
{
  SCPLUGIN_URI "#stereo_record",
  SCrecord::instantiate_st,
  SCrecord::connect_port,
  SCrecord::activate,
  SCrecord::run_st,
  SCrecord::deactivate,
  SCrecord::cleanup,
  NULL
};

////////////////////////// LV2 SYMBOL EXPORT ///////////////////////////

extern "C"
LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
  switch (index)
    {
    case 0:
      return &SCrecord::descriptor;
    case 1:
      return &SCrecord::descriptor1;
    default:
      return NULL;
    }
}

///////////////////////////// FIN //////////////////////////////////////
