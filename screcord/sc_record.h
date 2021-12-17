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

#include <lv2.h>
#include <lv2/lv2plug.in/ns/ext/state/state.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/options/options.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>

#define SCPLUGIN_URI "https://github.com/brummer10/screcord"
#define SCPLUGIN_UI_URI "https://github.com/brummer10/screcord_ui"

typedef enum
{
   FORM,
   REC,
   CLIP,
   LMETER,
   EFFECTS_OUTPUT,
   EFFECTS_INPUT,
   RMETER,
   EFFECTS_OUTPUT1,
   EFFECTS_INPUT1,
} PortIndex;
