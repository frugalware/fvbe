// fwsetup - flexible installer for Frugalware
// Copyright (C) 2013 James Buren
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include <newt.h>
#include "local.h"
 
union partition_action
{
  struct
  {
    unsigned char device_number;
    unsigned char partition_number;
    bool disk : 1;
    bool partition : 1;
    bool space : 1;
    bool delete : 1;
  };
  uintptr_t data;
};

static int lfind_compare(const void *A,const void *B)
{
  const char *a = (const char *) A;
  const struct tool *b = *(const struct tool **) B;
  size_t n = 0;
  
  for( ; b->name[n] != '.' && b->name[n] != 0 ; ++n )
    ;
  
  return strncmp(a,b->name,n);
}

static int lfind_compare_nm(const void *A,const void *B)
{
  const char *a = (const char *) A;
  const struct nmprofile *b = *(const struct nmprofile **) B;
  const char *c = (b->data == 0) ? "" : iniparser_getstring(b->data,PROFILE_NAME_KEY,"");
  
  return strcmp(a,c);
}

static inline bool findpath(struct format **targets,struct format *target,const char *path)
{
  struct format **p = targets;

  for( ; *p != 0 ; ++p )
  {
    struct format *t = *p;

    if(t == target)
      continue;

    if(t->newfilesystem == 0 && t->mountpath == 0)
      continue;

    if(strcmp(t->newfilesystem,"swap") == 0)
      continue;

    if(strcmp(t->mountpath,path) == 0)
      return true;
  }

  return false;
}

static int get_text_screen_width(const char *text)
{
  const char *s = 0;
  size_t len = 0;
  wchar_t *wcs = 0;
  size_t i = 0;
  wchar_t wc = 0;
  int wcw = 0;
  int w = 0;

  if(text == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return -1;
  }

  s = text;
  len = mbsrtowcs(NULL,&s,0,NULL);

  if(len == 0)
  {
    return 0;
  }
  else if(len == (size_t) -1)
  {
    error(strerror(errno));
    return -1;
  }

  wcs = alloc(wchar_t,len);
  s = text;

  if(mbsrtowcs(wcs,&s,len,NULL) == (size_t) -1)
  {
    error(strerror(errno));
    free(wcs);
    return -1;
  }

  for( ; i < len ; ++i )
  {
    wc = wcs[i];

    if(wc == L'\n')
    {
      break;
    }
    else if(wc == L'\t')
    {
      w += 8;
    }
    else if((wcw = wcwidth(wc)) > 0)
    {
      w += wcw;
    }
  }

  free(wcs);

  return w;
}

static bool get_text_screen_size(const char *text,int *width,int *height)
{
  const char *s = text;
  int cw = 0;
  int w = 0;
  int h = 0;

  if(text == 0 || width == 0 || height == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

  while(true)
  {
    cw = get_text_screen_width(s);

    if(cw == -1)
      return false;

    if(w < cw)
      w = cw;

    if((s = strchr(s,'\n')) == 0)
      break;

    ++h;

    ++s;
  }

  *width = w;

  *height = h;

  return true;
}

static bool get_button_screen_size(const char *text,int *width,int *height)
{
  int w = 0;
  int h = 0;

  if(text == 0 || width == 0 || height == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

  if((w = get_text_screen_width(text)) == -1)
    return false;

  w += 5;

  h = 4;

  *width = w;

  *height = h;

  return true;
}

static bool get_label_screen_size(const char *text,int *width,int *height)
{
  int w = 0;
  int h = 0;

  if(text == 0 || width == 0 || height == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

  if((w = get_text_screen_width(text)) == -1)
    return false;

  h = 1;

  *width = w;

  *height = h;

  return true;
}

static bool get_checkbox_screen_size(const char *text,int *width,int *height)
{
  int w = 0;
  int h = 0;

  if(text == 0 || width == 0 || height == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

  if((w = get_text_screen_width(text)) == -1)
    return false;

  w += 4;

  h = 1;

  *width = w;

  *height = h;

  return true;
}

static bool process_ini_list(char *list,bool (*f) (const char *),int m,int r)
{
  char *s = list;
  char *e = 0;
  
  for( ; (e = strchr(s,m)) != 0 ; s = e + 1 )
  {
    *e = 0;
    
    if(!f(s))
    {
      eprintf("%s: invalid data '%s'\n",__func__,s);
      return false;
    }
    
    if(e[1] == 0)
      break;
    
    *e = r;
  }

  if(!f(s))
  {
    eprintf("%s: invalid data '%s'\n",__func__,s);
    return false;
  }

  return true;
}

static void ui_dialog_text(const char *title,const char *text)
{
  int textbox_width = 0;
  int textbox_height = 0;
  int button_width = 0;
  int button_height = 0;
  newtComponent textbox = 0;
  newtComponent button = 0;
  newtComponent form = 0;
  struct newtExitStruct es = {0};

  if(title == 0 || text == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return;
  }

  if(!get_text_screen_size(text,&textbox_width,&textbox_height))
    return;

  if(!get_button_screen_size(OK_BUTTON_TEXT,&button_width,&button_height))
    return;

  if(newtCenteredWindow(NEWT_WIDTH,NEWT_HEIGHT,title) != 0)
  {
    eprintf("Failed to open a NEWT window.\n");
    return;
  }

  textbox = newtTextbox(0,0,textbox_width,textbox_height,0);

  newtTextboxSetText(textbox,text);

  button = newtButton(NEWT_WIDTH-button_width,NEWT_HEIGHT-button_height,OK_BUTTON_TEXT);

  form = newtForm(0,0,NEWT_FLAG_NOF12);

  newtFormAddComponents(form,textbox,button,(void *) 0);

  newtFormSetCurrent(form,button);

  while(true)
  {
    newtFormRun(form,&es);

    if(es.reason == NEWT_EXIT_COMPONENT && es.u.co == button)
      break;
  }

  newtFormDestroy(form);

  newtPopWindow();
}

static bool ui_dialog_edit_profile(struct nmprofile *profile,struct nmdevice **devices,struct nmprofile **profiles,struct nmdevice **out)
{
  int textbox_width = 0;
  int textbox_height = 0;
  int label_width = 0;
  int label_height = 0;
  int entry_left = 0;
  int entry_width = 0;
  int listbox_width = 0;
  int listbox_height = 0;
  int next_width = 0;
  int next_height = 0;
  char *p = 0;
  char buf[TEXT_MAX] = {0};
  bool found = false;
  const char *name = 0;
  const char *mac = 0;
  newtComponent textbox = 0;
  newtComponent label = 0;
  newtComponent entry = 0;
  newtComponent listbox = 0;
  newtComponent next = 0;
  newtComponent form = 0;
  struct newtExitStruct es = {0};
  
  if(!get_text_screen_size(NM_PROFILE_TEXT,&textbox_width,&textbox_height))
    return false;
  
  if(!get_label_screen_size(PROFILE_NAME_TEXT,&label_width,&label_height))
    return false;

  entry_left = label_width + 1;
  
  entry_width = NEWT_WIDTH - entry_left;
  
  if(!get_button_screen_size(NEXT_BUTTON_TEXT,&next_width,&next_height))
    return false;
  
  listbox_width = NEWT_WIDTH;
  
  listbox_height = NEWT_HEIGHT - textbox_height - label_height - next_height - 3;
  
  p = iniparser_getstring(profile->data,PROFILE_NAME_KEY,"");
  
  if(strlen(p) > 0)
    name = strdupa(p);
  
  p = iniparser_getstring(profile->data,PROFILE_TYPE_KEY,"");
  
  if(strcmp(p,WIRED_KEY) == 0 || strcmp(p,WIFI_KEY) == 0)
  {
    strfcpy(buf,sizeof(buf),"%s:mac-address",p);
    
    p = iniparser_getstring(profile->data,buf,"");
    
    if(is_mac_address(p))
      mac = strdupa(p);
  }

  if(newtCenteredWindow(NEWT_WIDTH,NEWT_HEIGHT,NM_PROFILE_TITLE) != 0)
  {
    eprintf("Failed to open a NEWT window.\n");
    return false;
  }
  
  textbox = newtTextbox(0,0,textbox_width,textbox_height,0);

  newtTextboxSetText(textbox,NM_PROFILE_TEXT);
  
  label = newtLabel(0,textbox_height+1,PROFILE_NAME_TEXT);

  entry = newtEntry(entry_left,textbox_height+1,strng(name),entry_width,&name,NEWT_FLAG_SCROLL);

  listbox = newtListbox(0,textbox_height+label_height+2,listbox_height,NEWT_FLAG_SCROLL);

  newtListboxSetWidth(listbox,listbox_width);

  for( int i = 0 ; devices[i] != 0 ; ++i )
  {
    struct nmdevice *device = devices[i];
  
    if(strcmp(device->type,WIRED_KEY) == 0 || strcmp(device->type,WIFI_KEY) == 0)
    {
      strfcpy(buf,sizeof(buf),"%-16.16s %.*s",device->device,NEWT_WIDTH-18,device->product);
    
      newtListboxAppendEntry(listbox,buf,device);
      
      if(!found && strcmp(device->hwaddr,strng(mac)) == 0)
      {
        newtListboxSetCurrentByKey(listbox,device);
        
        found = true;
      }
    }
  }

  if(!found)
    newtListboxSetCurrent(listbox,0);

  next = newtButton(NEWT_WIDTH-next_width,NEWT_HEIGHT-next_height,NEXT_BUTTON_TEXT);
  
  form = newtForm(0,0,NEWT_FLAG_NOF12);

  newtFormAddComponents(form,textbox,label,entry,listbox,next,(void *) 0);

  newtFormSetCurrent(form,entry);

  while(true)
  {
    newtFormRun(form,&es);

    if(es.reason == NEWT_EXIT_COMPONENT && es.u.co == next)
    {
      struct nmdevice *device = newtListboxGetCurrent(listbox);
      
      found = false;
      
      for( int i = 0 ; profiles[i] != 0 ; ++i )
      {
        struct nmprofile *p = profiles[i];
        const char *name2 = iniparser_getstring(p->data,PROFILE_NAME_KEY,"");
        
        if(p != profile && strcmp(name,name2) == 0)
        {
          found = true;
        
          break;
        }
      }
      
      if(
        strlen(name) == 0     ||
        strchr(name,'/') != 0 ||
        found
      )
      {
        ui_dialog_text(NM_PROFILE_ERROR_TITLE,NM_PROFILE_ERROR_TEXT);
        continue;
      }
      
      iniparser_unset_section(profile->data,PROFILE_KEY);
      
      iniparser_set(profile->data,PROFILE_KEY,"");
      
      iniparser_set(profile->data,PROFILE_NAME_KEY,name);
      
      iniparser_set(profile->data,PROFILE_UUID_KEY,uuidgen());
      
      iniparser_set(profile->data,PROFILE_TYPE_KEY,device->type);

      strfcpy(buf,sizeof(buf),"etc/NetworkManager/system-connections/%s",name);

      free(profile->newpath);
      
      profile->newpath = strdup(buf);

      *out = device;

      break;
    }
  }

  newtFormDestroy(form);

  newtPopWindow();
  
  return true;
}

static bool ui_dialog_static_ip(int type,struct nmprofile *profile)
{
  const char *iptype = 0;
  int prefixbits = 0;
  int prefixbitsmax = 0;
  bool (*vfun) (const char *) = 0;
  const char *ipkey = 0;
  const char *methodkey = 0;
  const char *addresskey = 0;
  const char *serverkey = 0;
  const char *searchkey = 0;
  char title[TEXT_MAX] = {0};
  char text[TEXT_MAX] = {0};
  int textbox_width = 0;
  int textbox_height = 0;
  int label1_width = 0;
  int label1_height = 0;
  int label2_width = 0;
  int label2_height = 0;
  int label3_width = 0;
  int label3_height = 0;
  int label4_width = 0;
  int label4_height = 0;
  int label5_width = 0;
  int label5_height = 0;
  int entry_left = 0;
  int entry_width = 0;
  int next_width = 0;
  int next_height = 0;
  char *p = 0;
  char *s = 0;
  char *e = 0;
  const char *address = 0;
  const char *prefix = 0;
  const char *gateway = 0;
  const char *servers = 0;
  const char *domains = 0;
  newtComponent textbox = 0;
  newtComponent label1 = 0;
  newtComponent entry1 = 0;
  newtComponent label2 = 0;
  newtComponent entry2 = 0;
  newtComponent label3 = 0;
  newtComponent entry3 = 0;
  newtComponent label4 = 0;
  newtComponent entry4 = 0;
  newtComponent label5 = 0;
  newtComponent entry5 = 0;
  newtComponent next = 0;
  newtComponent form = 0;
  struct newtExitStruct es = {0};

  switch(type)
  {
    case 4:
      iptype = "IPv4";
      prefixbits = 24;
      prefixbitsmax = 32;
      vfun = is_ip_v4;
      ipkey = "ipv4";
      methodkey = "ipv4:method";
      addresskey = "ipv4:address1";
      serverkey = "ipv4:dns";
      searchkey = "ipv4:dns-search";
      break;
    
    case 6:
      iptype = "IPv6";
      prefixbits = 64;
      prefixbitsmax = 128;
      vfun = is_ip_v6;
      ipkey = "ipv6";
      methodkey = "ipv6:method";
      addresskey = "ipv6:address1";
      serverkey = "ipv6:dns";
      searchkey = "ipv6:dns-search";
      break;
    
    default:
      eprintf("%s: unknown ip type %d\n",__func__,type);
      break;
  }

  strfcpy(title,sizeof(title),STATIC_IP_TITLE,iptype);  

  strfcpy(text,sizeof(text),STATIC_IP_TEXT,iptype,iptype,prefixbits,iptype,iptype,iptype);

  if(!get_text_screen_size(text,&textbox_width,&textbox_height))
    return false;

  if(!get_label_screen_size(ADDRESS_TEXT,&label1_width,&label1_height))
    return false;
  
  if(!get_label_screen_size(PREFIX_TEXT,&label2_width,&label2_height))
    return false;
  
  if(!get_label_screen_size(GATEWAY_TEXT,&label3_width,&label3_height))
    return false;
  
  if(!get_label_screen_size(DNS_SERVERS_TEXT,&label4_width,&label4_height))
    return false;
  
  if(!get_label_screen_size(SEARCH_DOMAINS_TEXT,&label5_width,&label5_height))
    return false;

  entry_left = maxv( (long long []) { label1_width, label2_width, label3_width, label4_width, label5_width }, 5 ) + 1;

  entry_width = NEWT_WIDTH - entry_left;

  if(!get_button_screen_size(NEXT_BUTTON_TEXT,&next_width,&next_height))
    return false;

  p = iniparser_getstring(profile->data,addresskey,"");
  
  if(strlen(p) > 0)
  {
    s = p;
    
    if((e = strchr(s,'/')) == 0)
    {
      eprintf("%s: invalid %s address\n",__func__,ipkey);
      return false;
    }
    
    address = strndupa(s,e-s);
    
    if(!vfun(address))
    {
      eprintf("%s: invalid %s address '%s'\n",__func__,ipkey,address);
      return false;
    }
    
    s = e + 1;
    
    if((e = strchr(s,',')) == 0)
    {
      eprintf("%s: invalid %s address\n",__func__,ipkey);
      return false;
    }

    prefix = strndupa(s,e-s);
    
    if(!is_positive_integer(prefix) || atoi(prefix) > prefixbitsmax)
    {
      eprintf("%s: invalid %s prefix '%s'\n",__func__,ipkey,prefix);
      return false;
    }

    s = e + 1;

    gateway = strdupa(s);
    
    if(!vfun(gateway))
    {
      eprintf("%s: invalid %s gateway '%s'\n",__func__,ipkey,gateway);
      return false;
    }    
  }

  p = iniparser_getstring(profile->data,serverkey,"");

  if(strlen(p) > 0)
  {
    servers = p = strdupa(p);
    
    if(!process_ini_list(p,vfun,';',','))
      return false;
  }

  p = iniparser_getstring(profile->data,searchkey,"");

  if(strlen(p) > 0)
  {
    domains = p = strdupa(p);
    
    if(!process_ini_list(p,is_dns_domain,';',','))
      return false;
  }

  if(newtCenteredWindow(NEWT_WIDTH,NEWT_HEIGHT,title) != 0)
  {
    eprintf("Failed to open a NEWT window.\n");
    return false;
  }

  textbox = newtTextbox(0,0,textbox_width,textbox_height,0);

  newtTextboxSetText(textbox,text);

  label1 = newtLabel(0,textbox_height+1,ADDRESS_TEXT);

  entry1 = newtEntry(entry_left,textbox_height+1,strng(address),entry_width,&address,NEWT_FLAG_SCROLL);

  label2 = newtLabel(0,textbox_height+label1_height+2,PREFIX_TEXT);

  entry2 = newtEntry(entry_left,textbox_height+label1_height+2,strng(prefix),entry_width,&prefix,NEWT_FLAG_SCROLL);

  label3 = newtLabel(0,textbox_height+label1_height+label2_height+3,GATEWAY_TEXT);

  entry3 = newtEntry(entry_left,textbox_height+label1_height+label2_height+3,strng(gateway),entry_width,&gateway,NEWT_FLAG_SCROLL);

  label4 = newtLabel(0,textbox_height+label1_height+label2_height+label3_height+4,DNS_SERVERS_TEXT);

  entry4 = newtEntry(entry_left,textbox_height+label1_height+label2_height+label3_height+4,strng(servers),entry_width,&servers,NEWT_FLAG_SCROLL);

  label5 = newtLabel(0,textbox_height+label1_height+label2_height+label3_height+label4_height+5,SEARCH_DOMAINS_TEXT);

  entry5 = newtEntry(entry_left,textbox_height+label1_height+label2_height+label3_height+label4_height+5,strng(domains),entry_width,&domains,NEWT_FLAG_SCROLL);

  next = newtButton(NEWT_WIDTH-next_width,NEWT_HEIGHT-next_height,NEXT_BUTTON_TEXT);

  form = newtForm(0,0,NEWT_FLAG_NOF12);

  newtFormAddComponents(form,textbox,label1,entry1,label2,entry2,label3,entry3,label4,entry4,label5,entry5,next,(void *) 0);

  newtFormSetCurrent(form,entry1);

  while(true)
  {
    newtFormRun(form,&es);

    if(es.reason == NEWT_EXIT_COMPONENT && es.u.co == next)
    {
      bool pass = false;
      char address2[TEXT_MAX] = {0};
      char servers2[TEXT_MAX] = {0};
      char domains2[TEXT_MAX] = {0};
      
      if(!vfun(address))
        goto fail;
      
      if(!is_positive_integer(prefix) || atoi(prefix) > prefixbitsmax)
        goto fail;
      
      if(!vfun(gateway))
        goto fail;
    
      strfcpy(address2,sizeof(address2),"%s/%s,%s",address,prefix,gateway);
    
      strfcpy(servers2,sizeof(servers2),"%s",servers);
    
      if(!process_ini_list(servers2,vfun,',',';'))
        goto fail;

      strfcat(servers2,sizeof(servers2),";");
      
      if(strlen(domains) > 0)
      {
        strfcpy(domains2,sizeof(domains2),"%s",domains);
      
        if(!process_ini_list(domains2,is_dns_domain,',',';'))
          goto fail;

        strfcat(domains2,sizeof(domains2),";");
      }

      pass = true;
      
      fail:
      
      if(!pass)
      {
        strfcpy(title,sizeof(title),STATIC_IP_ERROR_TITLE,iptype);
        strfcpy(text,sizeof(text),STATIC_IP_ERROR_TEXT,iptype,prefixbitsmax,iptype,iptype);
        ui_dialog_text(title,text);
        continue;
      }
    
      iniparser_unset_section(profile->data,ipkey);
      
      iniparser_set(profile->data,ipkey,"");
      
      iniparser_set(profile->data,methodkey,"manual");
      
      iniparser_set(profile->data,addresskey,address2);
      
      iniparser_set(profile->data,serverkey,servers2);
      
      if(strlen(domains) > 0)
        iniparser_set(profile->data,searchkey,domains2);
    
      break;
    }
  }

  newtFormDestroy(form);

  newtPopWindow();

  return true;
}

static bool ui_dialog_wifi(struct nmprofile *profile,struct nmdevice *device)
{
  int textbox_width = 0;
  int textbox_height = 0;
  int label1_width = 0;
  int label1_height = 0;
  int label2_width = 0;
  int label2_height = 0;
  int entry_left = 0;
  int entry_width = 0;
  int next_width = 0;
  int next_height = 0;
  char *p = 0;
  char buf[TEXT_MAX] = {0};
  const char *ssid = 0;
  const char *psk = 0;
  newtComponent textbox = 0;
  newtComponent label1 = 0;
  newtComponent entry1 = 0;
  newtComponent label2 = 0;
  newtComponent entry2 = 0;
  newtComponent next = 0;
  newtComponent form = 0;
  struct newtExitStruct es = {0};
  
  if(!get_text_screen_size(NM_WIFI_TEXT,&textbox_width,&textbox_height))
    return false;
  
  if(!get_label_screen_size(SSID_TEXT,&label1_width,&label1_height))
    return false;

  if(!get_label_screen_size(PSK_TEXT,&label2_width,&label2_height))
    return false;

  entry_left = max(label1_width,label2_width) + 1;
  
  entry_width = NEWT_WIDTH - entry_left;

  if(!get_button_screen_size(NEXT_BUTTON_TEXT,&next_width,&next_height))
    return false;

  p = iniparser_getstring(profile->data,WIFI_KEY ":ssid","");

  if(is_wifi_ssid(p))
    ssid = strdupa(p);

  p = iniparser_getstring(profile->data,WIFI_KEY ":security","");
  
  if(strlen(p) > 0)
  {
    char *base = strdupa(p);
  
    strfcpy(buf,sizeof(buf),"%s:key-mgmt",base);
    
    p = iniparser_getstring(profile->data,buf,"");
    
    if(strcmp(p,"wpa-psk") == 0)
    {
      strfcpy(buf,sizeof(buf),"%s:psk",base);
    
      p = iniparser_getstring(profile->data,buf,"");
      
      if(is_wpa_psk(p))
      {
        strfcpy(buf,sizeof(buf),"wpa:%s",p);
        
        psk = strdupa(buf);
      }
    }
    else if(strcmp(p,"none") == 0)
    {
      strfcpy(buf,sizeof(buf),"%s:wep-key-type",base);
      
      p = iniparser_getstring(profile->data,buf,"");
      
      if(strcmp(p,"1") == 0)
      {
        strfcpy(buf,sizeof(buf),"%s:wep-key0",base);
      
        p = iniparser_getstring(profile->data,buf,"");
        
        if(is_wep_psk(p))
        {
          strfcpy(buf,sizeof(buf),"wep:%s",p);
          
          psk = strdupa(buf);
        }
      }
    }
  }

  if(newtCenteredWindow(NEWT_WIDTH,NEWT_HEIGHT,NM_WIFI_TITLE) != 0)
  {
    eprintf("Failed to open a NEWT window.\n");
    return false;
  }
  
  textbox = newtTextbox(0,0,textbox_width,textbox_height,0);

  newtTextboxSetText(textbox,NM_WIFI_TEXT);
  
  label1 = newtLabel(0,textbox_height+1,SSID_TEXT);

  entry1 = newtEntry(entry_left,textbox_height+1,strng(ssid),entry_width,&ssid,NEWT_FLAG_SCROLL);

  label2 = newtLabel(0,textbox_height+label1_height+2,PSK_TEXT);

  entry2 = newtEntry(entry_left,textbox_height+label1_height+2,strng(psk),entry_width,&psk,NEWT_FLAG_SCROLL);

  next = newtButton(NEWT_WIDTH-next_width,NEWT_HEIGHT-next_height,NEXT_BUTTON_TEXT);

  form = newtForm(0,0,NEWT_FLAG_NOF12);

  newtFormAddComponents(form,textbox,label1,entry1,label2,entry2,next,(void *) 0);

  newtFormSetCurrent(form,entry1);

  while(true)
  {
    newtFormRun(form,&es);

    if(es.reason == NEWT_EXIT_COMPONENT && es.u.co == next)
    {
      bool pass = false;
      bool wpa = false;
      bool wep = false;

      if(!is_wifi_ssid(ssid))
        goto fail;
    
      if(strlen(psk) > 0)
      {
        if(strncmp(psk,"wpa:",4) == 0)
        {
          if(!is_wpa_psk(psk+4))
            goto fail;
          
          if(!device->wpa && !device->wpa2)
            goto fail;
            
          wpa = true;
        }
        else if(strncmp(psk,"wep:",4) == 0)
        {
          if(!is_wep_psk(psk+4))
            goto fail;
          
          if(!device->wep)
            goto fail;
          
          wep = true;
        }
        else
          goto fail;
      }
    
      pass = true;
    
      fail:
    
      if(!pass)
      {
        ui_dialog_text(NM_WIFI_ERROR_TITLE,NM_WIFI_ERROR_TEXT);
        continue;
      }
    
      iniparser_unset_section(profile->data,WIFI_KEY);
      
      iniparser_set(profile->data,WIFI_KEY,"");
      
      iniparser_set(profile->data,WIFI_KEY ":mode","infrastructure");
      
      iniparser_set(profile->data,WIFI_KEY ":ssid",ssid);
    
      iniparser_unset_section(profile->data,WIFI_KEY "-security");
    
      if(wpa || wep)
      {
        iniparser_set(profile->data,WIFI_KEY "-security","");
        
        iniparser_set(profile->data,WIFI_KEY ":security",WIFI_KEY "-security");
 
        if(wpa)
        {
          iniparser_set(profile->data,WIFI_KEY "-security:key-mgmt","wpa-psk");
          
          iniparser_set(profile->data,WIFI_KEY "-security:psk",psk+4);
        }
        else if(wep)
        {
          iniparser_set(profile->data,WIFI_KEY "-security:key-mgmt","none");

          iniparser_set(profile->data,WIFI_KEY "-security:auth-alg","open");

          iniparser_set(profile->data,WIFI_KEY "-security:wep-key-type","1");

          iniparser_set(profile->data,WIFI_KEY "-security:wep-key0",psk+4);
        }
      }
      
      iniparser_unset_section(profile->data,WIRED_KEY);
      
      break;
    }
  }


  newtFormDestroy(form);

  newtPopWindow();

  return true;
}

static inline bool process_nm_profile(struct nmprofile *profile,struct nmdevice **devices,struct nmprofile **profiles)
{
  struct nmdevice *device = 0;
  char buf[TEXT_MAX] = {0};
  char title[TEXT_MAX] = {0};
  char text[TEXT_MAX] = {0};

  if(!ui_dialog_edit_profile(profile,devices,profiles,&device))
    return false;

  if(strcmp(device->type,WIFI_KEY) == 0 && !ui_dialog_wifi(profile,device))
    return false;
  else if(strcmp(device->type,WIRED_KEY) == 0)
  {
    iniparser_unset_section(profile->data,WIRED_KEY);
    
    iniparser_set(profile->data,WIRED_KEY,"");
    
    iniparser_unset_section(profile->data,WIFI_KEY);
  
    iniparser_unset_section(profile->data,WIFI_KEY "-security");
  }

  strfcpy(buf,sizeof(buf),"%s:mac-address",device->type);

  iniparser_set(profile->data,buf,device->hwaddr);

  strfcpy(title,sizeof(title),NM_DHCP_TITLE,"IPv4");
  
  strfcpy(text,sizeof(text),NM_DHCP_TEXT,"IPv4");

  if(!ui_dialog_yesno(title,text,false))
  {
    if(!ui_dialog_static_ip(4,profile))
      return false;
  }
  else
  {
    iniparser_unset_section(profile->data,"ipv4");
    
    iniparser_set(profile->data,"ipv4","");
    
    iniparser_set(profile->data,"ipv4:method","auto");
  }

  strfcpy(title,sizeof(title),NM_ASK_IP_TITLE,"IPv6");
  
  strfcpy(text,sizeof(text),NM_ASK_IP_TEXT,"IPv6");

  if(ui_dialog_yesno(title,text,true))
  {
    strfcpy(title,sizeof(title),NM_DHCP_TITLE,"IPv6");
  
    strfcpy(text,sizeof(text),NM_DHCP_TEXT,"IPv6");

    if(!ui_dialog_yesno(title,text,false))
    {
      if(!ui_dialog_static_ip(6,profile))
        return false;
    }
    else
    {
      iniparser_unset_section(profile->data,"ipv6");
    
      iniparser_set(profile->data,"ipv6","");
    
      iniparser_set(profile->data,"ipv6:method","auto");
    }
  }
  else
  {
    iniparser_unset_section(profile->data,"ipv6");
    
    iniparser_set(profile->data,"ipv6","");
    
    iniparser_set(profile->data,"ipv6:method","ignore");
  }

  return true;
}

static bool ui_dialog_format(struct format **targets,struct format *target)
{
  int textbox_width = 0;
  int textbox_height = 0;
  int label1_width = 0;
  int label1_height = 0;
  int listbox_width = 0;
  int listbox_height = 0;
  int ok_width = 0;
  int ok_height = 0;
  int cancel_width = 0;
  int cancel_height = 0;
  int entry_left = 0;
  int entry_width = 0;
  newtComponent textbox = 0;
  newtComponent label1 = 0;
  newtComponent entry1 = 0;
  newtComponent listbox = 0;
  newtComponent cancel = 0;
  newtComponent ok = 0;
  const char *path = 0;
  static const char *filesystems[] =
  {
    "noformat",
    "ext2",
    "ext3",
    "ext4",
    "xfs",
    "btrfs",
    "swap",
    0
  };
  const char **p = 0;
  newtComponent form = 0;
  struct newtExitStruct es = {0};

  if(!get_text_screen_size(FORMAT_DIALOG_TEXT,&textbox_width,&textbox_height))
    return false;

  if(!get_label_screen_size(FORMAT_MOUNT_ENTRY_TEXT,&label1_width,&label1_height))
    return false;

  if(!get_button_screen_size(OK_BUTTON_TEXT,&ok_width,&ok_height))
    return false;

  if(!get_button_screen_size(CANCEL_BUTTON_TEXT,&cancel_width,&cancel_height))
    return false;

  entry_left = label1_width + 1;

  entry_width = NEWT_WIDTH - entry_left;

  listbox_width = NEWT_WIDTH;

  listbox_height = NEWT_HEIGHT - textbox_height - label1_height - ok_height - 3;

  if(newtCenteredWindow(NEWT_WIDTH,NEWT_HEIGHT,FORMAT_TITLE) != 0)
  {
    eprintf("Failed to open a NEWT window.\n");
    return false;
  }

  textbox = newtTextbox(0,0,textbox_width,textbox_height,0);

  newtTextboxSetText(textbox,FORMAT_DIALOG_TEXT);

  label1 = newtLabel(0,textbox_height+1,FORMAT_MOUNT_ENTRY_TEXT);

  entry1 = newtEntry(entry_left,textbox_height+1,(target->mountpath != 0) ? target->mountpath : g->hostroot,entry_width,&path,NEWT_FLAG_SCROLL);

  listbox = newtListbox(0,textbox_height+label1_height+2,listbox_height,NEWT_FLAG_SCROLL);

  newtListboxSetWidth(listbox,listbox_width);

  for( p = filesystems ; *p != 0 ; ++p )
  {
    newtListboxAppendEntry(listbox,*p,*p);
  }

  if(target->format && target->newfilesystem != 0)
  {
    for( p = filesystems ; *p != 0 ; ++p )
    {
      if(strcmp(*p,target->newfilesystem) == 0)
      {
        newtListboxSetCurrentByKey(listbox,(void *) *p);
        break;
      }
    }
  }
  else
  {
    newtListboxSetCurrentByKey(listbox,(void *) filesystems[0]);
  }

  ok = newtButton(NEWT_WIDTH-cancel_width-ok_width,NEWT_HEIGHT-cancel_height,OK_BUTTON_TEXT);

  cancel = newtButton(NEWT_WIDTH-cancel_width,NEWT_HEIGHT-cancel_height,CANCEL_BUTTON_TEXT);

  form = newtForm(0,0,NEWT_FLAG_NOF12);

  newtFormAddComponents(form,textbox,label1,entry1,listbox,ok,cancel,(void *) 0);

  newtFormSetCurrent(form,entry1);

  while(true)
  {
    newtFormRun(form,&es);

    if(es.reason == NEWT_EXIT_COMPONENT && es.u.co == cancel)
    {
      break;
    }
    else if(es.reason == NEWT_EXIT_COMPONENT && es.u.co == ok)
    {
      const char *filesystem = newtListboxGetCurrent(listbox);

      if(
        (strcmp(filesystem,"noformat") == 0 && strcmp(target->filesystem,"unknown") == 0)        ||
        (strcmp(filesystem,"swap") != 0 && (!is_root_path(path) || findpath(targets,target,path)))
      )
      {
        ui_dialog_text(FORMAT_PATH_TITLE,FORMAT_PATH_TEXT);
        continue;
      }

      free(target->newfilesystem);

      free(target->mountpath);

      target->format = (strcmp(filesystem,"noformat") != 0);

      target->newfilesystem = strdup( (target->format) ? filesystem : target->filesystem );

      target->mountpath = strdup(path);

      break;
    }
  }

  newtFormDestroy(form);

  newtPopWindow();

  return true;
}

static bool ui_dialog_partition_new_table(struct device *device,struct disk **data)
{
  int textbox_width = 0;
  int textbox_height = 0;
  int ok_width = 0;
  int ok_height = 0;
  int cancel_width = 0;
  int cancel_height = 0;
  int listbox_width = 0;
  int listbox_height = 0;
  newtComponent textbox = 0;
  newtComponent ok = 0;
  newtComponent cancel = 0;
  newtComponent listbox = 0;
  newtComponent form = 0;
  struct newtExitStruct es = {0};
  bool modified = false;

  if(!get_text_screen_size(PARTITION_DIALOG_NEW_TABLE_TEXT,&textbox_width,&textbox_height))
    return false;

  if(!get_button_screen_size(OK_BUTTON_TEXT,&ok_width,&ok_height))
    return false;

  if(!get_button_screen_size(CANCEL_BUTTON_TEXT,&cancel_width,&cancel_height))
    return false;

  listbox_width = NEWT_WIDTH;

  listbox_height = NEWT_HEIGHT - textbox_height - ok_height - 2;

  if(newtCenteredWindow(NEWT_WIDTH,NEWT_HEIGHT,PARTITION_DIALOG_NEW_TABLE_TITLE) != 0)
  {
    eprintf("Failed to open a NEWT window.\n");
    return false;
  }

  textbox = newtTextbox(0,0,textbox_width,textbox_height,0);

  newtTextboxSetText(textbox,PARTITION_DIALOG_NEW_TABLE_TEXT);

  ok = newtButton(NEWT_WIDTH-ok_width-cancel_width,NEWT_HEIGHT-ok_height,OK_BUTTON_TEXT);

  cancel = newtButton(NEWT_WIDTH-cancel_width,NEWT_HEIGHT-cancel_height,CANCEL_BUTTON_TEXT);

  listbox = newtListbox(0,textbox_height+1,listbox_height,NEWT_FLAG_SCROLL);

  newtListboxSetWidth(listbox,listbox_width);

  newtListboxAppendEntry(listbox,"dos","dos");

  newtListboxAppendEntry(listbox,"gpt","gpt");

  newtListboxSetCurrent(listbox,0);

  form = newtForm(0,0,NEWT_FLAG_NOF12);

  newtFormAddComponents(form,textbox,ok,cancel,listbox,(void *) 0);

  newtFormSetCurrent(form,listbox);

  while(true)
  {
    newtFormRun(form,&es);

    if(es.reason == NEWT_EXIT_COMPONENT && es.u.co == cancel)
    {
      break;
    }
    else if(es.reason == NEWT_EXIT_COMPONENT && es.u.co == ok)
    {
      const char *type = newtListboxGetCurrent(listbox);
      struct disk *disk = *data;

      if(disk == 0)
      {
        disk = disk_open_empty(device,type);
      }
      else
      {
        disk_new_table(disk,type);
      }

      *data = disk;

      modified = true;

      break;
    }
  }

  newtFormDestroy(form);

  newtPopWindow();

  return modified;
}

static bool ui_dialog_partition_modify_partition(struct disk *disk,int n)
{
  int textbox_width = 0;
  int textbox_height = 0;
  int ok_width = 0;
  int ok_height = 0;
  int cancel_width = 0;
  int cancel_height = 0;
  int checkbox_width = 0;
  int checkbox_height = 0;
  int label_width = 0;
  int label_height = 0;
  int entry_left = 0;
  int entry_width = 0;
  int listbox_width = 0;
  int listbox_height = 0;
  newtComponent textbox = 0;
  newtComponent ok = 0;
  newtComponent cancel = 0;
  newtComponent checkbox = 0;
  newtComponent label = 0;
  newtComponent entry = 0;
  newtComponent listbox = 0;
  newtComponent form = 0;
  struct newtExitStruct es = {0};
  bool modified = false;
  char active = ' ';
  const char *name = 0;
  int i = 0;
  static const char *purposes[] =
  {
    "data",
    "swap",
    "raid",
    "lvm",
    "efi",
    "bios",
    "unknown",
    0
  };
  const char *purpose = 0;

  if(!get_text_screen_size(PARTITION_DIALOG_MODIFY_PARTITION_TEXT,&textbox_width,&textbox_height))
    return false;

  if(!get_button_screen_size(OK_BUTTON_TEXT,&ok_width,&ok_height))
    return false;

  if(!get_button_screen_size(CANCEL_BUTTON_TEXT,&cancel_width,&cancel_height))
    return false;

  if(!get_checkbox_screen_size(PARTITION_DIALOG_MODIFY_PARTITION_ACTIVE_TEXT,&checkbox_width,&checkbox_height))
    return false;

  if(!get_label_screen_size(PARTITION_DIALOG_MODIFY_PARTITION_NAME_TEXT,&label_width,&label_height))
    return false;

  entry_left = label_width + 1;

  entry_width = NEWT_WIDTH - entry_left;

  listbox_width = NEWT_WIDTH;

  listbox_height = NEWT_HEIGHT - textbox_height - ok_height - checkbox_height - label_height - 4;

  if(newtCenteredWindow(NEWT_WIDTH,NEWT_HEIGHT,PARTITION_DIALOG_MODIFY_PARTITION_TITLE) != 0)
  {
    eprintf("Failed to open a NEWT window.\n");
    return false;
  }

  textbox = newtTextbox(0,0,textbox_width,textbox_height,0);

  newtTextboxSetText(textbox,PARTITION_DIALOG_MODIFY_PARTITION_TEXT);

  ok = newtButton(NEWT_WIDTH-ok_width-cancel_width,NEWT_HEIGHT-ok_height,OK_BUTTON_TEXT);

  cancel = newtButton(NEWT_WIDTH-cancel_width,NEWT_HEIGHT-cancel_height,CANCEL_BUTTON_TEXT);

  active = (disk_partition_get_active(disk,n)) ? '*' : ' ';

  checkbox = newtCheckbox(0,textbox_height+label_height+2,PARTITION_DIALOG_MODIFY_PARTITION_ACTIVE_TEXT,active,0,&active);

  label = newtLabel(0,textbox_height+1,PARTITION_DIALOG_MODIFY_PARTITION_NAME_TEXT);

  name = (strcmp(disk_get_type(disk),"gpt") == 0) ? disk_partition_get_name(disk,n) : "";

  entry = newtEntry(entry_left,textbox_height+1,name,entry_width,&name,NEWT_FLAG_SCROLL);

  listbox = newtListbox(0,textbox_height+label_height+checkbox_height+3,listbox_height,NEWT_FLAG_SCROLL);

  newtListboxSetWidth(listbox,listbox_width);

  purpose = disk_partition_get_purpose(disk,n);

  for( i = 0 ; purposes[i] != 0 ; ++i )
  {
    newtListboxAppendEntry(listbox,purposes[i],purposes[i]);

    if(strcmp(purposes[i],purpose) == 0)
      newtListboxSetCurrentByKey(listbox,(void *) purposes[i]);
  }

  form = newtForm(0,0,NEWT_FLAG_NOF12);

  newtFormAddComponents(form,textbox,ok,cancel,checkbox,label,entry,listbox,(void *) 0);

  newtFormSetCurrent(form,entry);

  while(true)
  {
    newtFormRun(form,&es);

    if(es.reason == NEWT_EXIT_COMPONENT && es.u.co == cancel)
    {
      break;
    }
    else if(es.reason == NEWT_EXIT_COMPONENT && es.u.co == ok)
    {
      purpose = newtListboxGetCurrent(listbox);

      if(!is_partition_name(name) || (strcmp(disk_get_type(disk),"gpt") != 0 && strcmp(purpose,"bios") == 0))
      {
        ui_dialog_text(PARTITION_DIALOG_MODIFY_PARTITION_ERROR_TITLE,PARTITION_DIALOG_MODIFY_PARTITION_ERROR_TEXT);
        continue;
      }

      if(strcmp(disk_get_type(disk),"gpt") == 0)
        disk_partition_set_name(disk,n,name);

      disk_partition_set_active(disk,n,(active == '*'));

      disk_partition_set_purpose(disk,n,purpose);

      modified = true;

      break;
    }
  }

  newtFormDestroy(form);

  newtPopWindow();

  return modified;
}

static bool ui_dialog_partition_new_partition(struct disk *disk)
{
  int textbox_width = 0;
  int textbox_height = 0;
  int ok_width = 0;
  int ok_height = 0;
  int cancel_width = 0;
  int cancel_height = 0;
  int label_width = 0;
  int label_height = 0;
  int entry_left = 0;
  int entry_width = 0;
  int listbox_width = 0;
  int listbox_height = 0;
  newtComponent textbox = 0;
  newtComponent ok = 0;
  newtComponent cancel = 0;
  newtComponent label = 0;
  newtComponent entry = 0;
  newtComponent listbox = 0;
  newtComponent form = 0;
  struct newtExitStruct es = {0};
  bool modified = false;
  char text[TEXT_MAX] = {0};
  const char *result = 0;

  if(!get_text_screen_size(PARTITION_DIALOG_NEW_PARTITION_TEXT,&textbox_width,&textbox_height))
    return false;

  if(!get_button_screen_size(OK_BUTTON_TEXT,&ok_width,&ok_height))
    return false;

  if(!get_button_screen_size(CANCEL_BUTTON_TEXT,&cancel_width,&cancel_height))
    return false;

  if(!get_label_screen_size(PARTITION_DIALOG_NEW_SIZE_TEXT,&label_width,&label_height))
    return false;

  entry_left = label_width + 1;

  entry_width = NEWT_WIDTH - entry_left;

  listbox_width = NEWT_WIDTH;

  listbox_height = NEWT_HEIGHT - textbox_height - ok_height - label_height - 3;

  if(newtCenteredWindow(NEWT_WIDTH,NEWT_HEIGHT,PARTITION_DIALOG_NEW_PARTITION_TITLE) != 0)
  {
    eprintf("Failed to open a NEWT window.\n");
    return false;
  }

  textbox = newtTextbox(0,0,textbox_width,textbox_height,0);

  newtTextboxSetText(textbox,PARTITION_DIALOG_NEW_PARTITION_TEXT);

  ok = newtButton(NEWT_WIDTH-ok_width-cancel_width,NEWT_HEIGHT-ok_height,OK_BUTTON_TEXT);

  cancel = newtButton(NEWT_WIDTH-cancel_width,NEWT_HEIGHT-cancel_height,CANCEL_BUTTON_TEXT);

  label = newtLabel(0,textbox_height+1,PARTITION_DIALOG_NEW_SIZE_TEXT);

  size_to_string(text,sizeof(text),disk_get_free_size(disk),false);

  entry = newtEntry(entry_left,textbox_height+1,text,entry_width,&result,NEWT_FLAG_SCROLL);

  listbox = newtListbox(0,textbox_height+label_height+2,listbox_height,NEWT_FLAG_SCROLL);

  newtListboxSetWidth(listbox,listbox_width);

  if(strcmp(disk_get_type(disk),"dos") == 0)
  {
    if(!disk_has_extended_partition(disk))
    {
      newtListboxAppendEntry(listbox,"primary","primary");
      newtListboxAppendEntry(listbox,"extended","extended");
    }
    else
    {
      newtListboxAppendEntry(listbox,"logical","logical");
    }
  }
  else
  {
    newtListboxAppendEntry(listbox,"primary","primary");
  }

  newtListboxSetCurrent(listbox,0);

  form = newtForm(0,0,NEWT_FLAG_NOF12);

  newtFormAddComponents(form,textbox,ok,cancel,label,entry,listbox,(void *) 0);

  newtFormSetCurrent(form,entry);

  while(true)
  {
    newtFormRun(form,&es);

    if(es.reason == NEWT_EXIT_COMPONENT && es.u.co == cancel)
    {
      break;
    }
    else if(es.reason == NEWT_EXIT_COMPONENT && es.u.co == ok)
    {
      long long size = string_to_size(result);
      const char *type = newtListboxGetCurrent(listbox);
      int n = 0;

      if(size == 0)
      {
        ui_dialog_text(PARTITION_DIALOG_NEW_PARTITION_ERROR_TITLE,PARTITION_DIALOG_NEW_PARTITION_ERROR_TEXT);
        continue;
      }

      if(strcmp(type,"primary") == 0)
        n = disk_create_partition(disk,size);
      else if(strcmp(type,"extended") == 0)
        n = disk_create_extended_partition(disk);
      else if(strcmp(type,"logical") == 0)
        n = disk_create_logical_partition(disk,size);

      if(n == -1)
      {
        ui_dialog_text(PARTITION_DIALOG_NEW_PARTITION_ERROR_TITLE,PARTITION_DIALOG_NEW_PARTITION_ERROR_TEXT);
        continue;
      }

      modified = true;

      break;
    }
  }

  newtFormDestroy(form);

  newtPopWindow();

  return modified;
}

static bool ui_dialog_raid(struct device ***unused,struct raid ***used,struct raid **raid)
{
  int textbox_width = 0;
  int textbox_height = 0;
  int ok_width = 0;
  int ok_height = 0;
  int cancel_width = 0;
  int cancel_height = 0;
  int levels_width = 0;
  int levels_height = 0;
  int devices_width = 0;
  int devices_height = 0;
  newtComponent textbox = 0;
  newtComponent ok = 0;
  newtComponent cancel = 0;
  newtComponent levels = 0;
  newtComponent devices = 0;
  newtComponent form = 0;
  struct newtExitStruct es = {0};
  char size[10] = {0};
  char text[TEXT_MAX] = {0};
  bool modified = false;

  if(!get_text_screen_size(RAID_DIALOG_NEW_TEXT,&textbox_width,&textbox_height))
    return false;

  if(!get_button_screen_size(OK_BUTTON_TEXT,&ok_width,&ok_height))
    return false;

  if(!get_button_screen_size(CANCEL_BUTTON_TEXT,&cancel_width,&cancel_height))
    return false;

  levels_width = NEWT_WIDTH;
  
  levels_height = 6;

  devices_width = NEWT_WIDTH;
  
  devices_height = NEWT_HEIGHT - textbox_height - ok_height - levels_height - 3;

  if(newtCenteredWindow(NEWT_WIDTH,NEWT_HEIGHT,RAID_DIALOG_NEW_TITLE) != 0)
  {
    eprintf("Failed to open a NEWT window.\n");
    return false;
  }

  textbox = newtTextbox(0,0,textbox_width,textbox_height,0);

  newtTextboxSetText(textbox,RAID_DIALOG_NEW_TEXT);

  ok = newtButton(NEWT_WIDTH-ok_width-cancel_width,NEWT_HEIGHT-ok_height,OK_BUTTON_TEXT);

  cancel = newtButton(NEWT_WIDTH-cancel_width,NEWT_HEIGHT-cancel_height,CANCEL_BUTTON_TEXT);

  levels = newtListbox(0,textbox_height+1,levels_height,NEWT_FLAG_SCROLL);

  newtListboxSetWidth(levels,levels_width);

  newtListboxAppendEntry(levels,"level0",(void *) 0);

  newtListboxAppendEntry(levels,"level1",(void *) 1);

  newtListboxAppendEntry(levels,"level4",(void *) 4);

  newtListboxAppendEntry(levels,"level5",(void *) 5);

  newtListboxAppendEntry(levels,"level6",(void *) 6);

  newtListboxAppendEntry(levels,"level10",(void *) 10);

  newtListboxSetCurrent(levels,0);

  devices = newtCheckboxTree(0,textbox_height+levels_height+2,devices_height,NEWT_FLAG_SCROLL);

  newtCheckboxTreeSetWidth(devices,devices_width);

  for( int i = 0 ; unused[0][i] != 0 ; ++i )
  {
    struct device *device = unused[0][i];
  
    size_to_string(size,sizeof(size),device_get_size(device),false);
  
    strfcpy(text,sizeof(text),"%s %s",device_get_path(device),size);
  
    newtCheckboxTreeAddItem(devices,text,device,0,i,NEWT_ARG_LAST);
  }

  form = newtForm(0,0,NEWT_FLAG_NOF12);

  newtFormAddComponents(form,textbox,ok,cancel,levels,devices,(void *) 0);

  newtFormSetCurrent(form,levels);

  while(true)
  {
    newtFormRun(form,&es);

    if(es.reason == NEWT_EXIT_COMPONENT && es.u.co == ok)
    {
      int level = (long) newtListboxGetCurrent(levels);
      int n = 0;
      struct device **disks = (struct device **) newtCheckboxTreeGetSelection(devices,&n);
      char path[PATH_MAX] = {0};

      if(raidmindisks(level) == -1 || disks == 0 || n < raidmindisks(level) || !find_unused_raid_device(*used,path,sizeof(path)))
      {
        ui_dialog_text(NO_RAID_DISKS_TITLE,NO_RAID_DISKS_TEXT);
        free(disks);
        continue;
      }
      
      modified = ((*raid = raid_open_empty(path,level,n,disks)) != 0);
      
      if(modified)
        update_raid_add(unused,used,*raid);
      
      free(disks);
      
      break;
    }
    else if(es.reason == NEWT_EXIT_COMPONENT && es.u.co == cancel)
    {
      break;
    }
  }

  newtFormDestroy(form);

  newtPopWindow();

  return modified;
}

extern int ui_main(int argc,char **argv)
{
  int w = 0;
  int h = 0;
  size_t n = 0;
  char text[TEXT_MAX] = {0};
  int code = EXIT_FAILURE;

  // This parameter is never used.
  argc = argc;

  // This parameter is never used.
  argv = argv;

  if(!isatty(STDIN_FILENO))
  {
    eprintf("stdin is not a tty.\n");
    return code;
  }

  if(!isatty(STDOUT_FILENO))
  {
    eprintf("stdout is not a tty.\n");
    return code;
  }

  if(!isatty(STDERR_FILENO))
  {
    eprintf("stderr is not a tty.\n");
    return code;
  }

  if(newtInit() != 0)
  {
    eprintf("Could not initialize the NEWT user interface.\n");
    return code;
  }

  // Needed to keep SIGWINCH from screwing up system calls.
  if(siginterrupt(SIGWINCH,false) == -1)
  {
    eprintf("Failed to stop SIGWINCH from interrupting system calls.\n");
    newtFinished();
    return code;
  }

  newtGetScreenSize(&w,&h);

  if(w < 80 || h < 24)
  {
    eprintf("We require a terminal of 80x24 or greater to use the NEWT user interface.\n");
    newtFinished();
    return code;
  }

  newtCls();

  klogctl(8,0,1);

  if(g->insetup)
  {
    struct module *module = 0;
  
    while(true)
    {
      module = modules[n];

      if(module == 0)
        break;

      if(module->run == 0 || module->reset == 0 || module->name == 0)
      {
        errno = EINVAL;
        error(strerror(errno));
        break;
      }

      eprintf("About to run module '%s'.\n",module->name);

      if(!module->run())
      {
        eprintf("A fatal error has been reported by module '%s'.\n",module->name);
        module->reset();
        strfcpy(text,sizeof(text),_("A fatal error has been reported by module '%s'.\nPlease read the logfile at '%s'.\nThank you.\n"),module->name,g->logpath);
        ui_dialog_text(_("Module Fatal Error"),text);
        break;
      }

      module->reset();

      ++n;
    }

    if(module == 0)
      code = EXIT_SUCCESS;
  }
  else
  {
    struct tool *tool = 0;

    n = tools_count;

    if(chdir(g->guestroot) == -1)
    {
      error(strerror(errno));
    }
    else if(
      (tool = lfind(g->name,tools,&n,sizeof(struct tool *),lfind_compare)) == 0  ||
      (tool = *(struct tool **) tool) == 0                                       ||
      tool->start == 0                                                           ||
      tool->finish == 0                                                          ||
      tool->name == 0
    )
    {
      errno = EINVAL;
      error(strerror(errno));
    }
    else
    {
      eprintf("About to run tool '%s'.\n",tool->name);
      
      if(!tool->start())
      {
        tool->finish();
        strfcpy(text,sizeof(text),_("A fatal error has been reported by tool '%s'.\nPlease read the logfile at '%s'.Thank you.\n"),tool->name,g->logpath);
        ui_dialog_text(_("Tool Fatal Error"),text);
      }
      else if(!tool->finish())
      {
        strfcpy(text,sizeof(text),_("A fatal error has been reported by tool '%s'.\nPlease read the logfile at '%s'.Thank you.\n"),tool->name,g->logpath);
        ui_dialog_text(_("Tool Fatal Error"),text);
      }
      else
      {
        code = EXIT_SUCCESS;
      }
    }
  }

  klogctl(8,0,4);

  newtFinished();

  return code;
}

extern bool ui_dialog_yesno(const char *title,const char *text,bool defaultno)
{
  int textbox_width = 0;
  int textbox_height = 0;
  int yes_width = 0;
  int yes_height = 0;
  int no_width = 0;
  int no_height = 0;
  newtComponent textbox = 0;
  newtComponent yes = 0;
  newtComponent no = 0;
  newtComponent form = 0;
  struct newtExitStruct es = {0};
  bool result = false;

  if(title == 0 || text == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

  if(!get_text_screen_size(text,&textbox_width,&textbox_height))
    return false;

  if(!get_button_screen_size(YES_BUTTON_TEXT,&yes_width,&yes_height))
    return false;

  if(!get_button_screen_size(NO_BUTTON_TEXT,&no_width,&no_height))
    return false;

  if(newtCenteredWindow(NEWT_WIDTH,NEWT_HEIGHT,title) != 0)
  {
    eprintf("Failed to open a NEWT window.\n");
    return false;
  }

  textbox = newtTextbox(0,0,textbox_width,textbox_height,0);

  newtTextboxSetText(textbox,text);

  yes = newtButton(NEWT_WIDTH-yes_width-no_width,NEWT_HEIGHT-yes_height,YES_BUTTON_TEXT);

  no = newtButton(NEWT_WIDTH-no_width,NEWT_HEIGHT-no_height,NO_BUTTON_TEXT);

  form = newtForm(0,0,NEWT_FLAG_NOF12);

  newtFormAddComponents(form,textbox,yes,no,(void *) 0);

  newtFormSetCurrent(form,(defaultno) ? no : yes);

  while(true)
  {
    newtFormRun(form,&es);

    if(es.reason == NEWT_EXIT_COMPONENT && (es.u.co == yes || es.u.co == no))
    {
      result = (es.u.co == yes);
      break;
    }
  }

  newtFormDestroy(form);

  newtPopWindow();

  return result;
}

extern bool ui_dialog_progress(const char *title,const char *text,int percent)
{
  static char *oldtitle = 0;
  static newtComponent label = 0;
  static newtComponent scale = 0;
  static newtComponent form = 0;

  if((title == 0 && text == 0 && percent == -1) || (oldtitle != 0 && strcmp(oldtitle,title) != 0))
  {
    if(label != 0 && scale != 0 && form != 0 && oldtitle != 0)
    {
      free(oldtitle);
      newtFormDestroy(form);
      newtPopWindow();
      oldtitle = 0;
      label = 0;
      scale = 0;
      form = 0;
    }

    if(title == 0 && text == 0 && percent == -1)
      return true;
  }

  if(title == 0 || text == 0 || percent < 0 || percent > 100)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

  if(label == 0 && scale == 0 && form == 0)
  {
    if(newtCenteredWindow(NEWT_WIDTH,3,title) != 0)
    {
      eprintf("Failed to open a NEWT window.\n");
      return false;
    }

    oldtitle = strdup(title);

    label = newtLabel(0,0,"");

    scale = newtScale(0,2,NEWT_WIDTH,100);

    form = newtForm(0,0,NEWT_FLAG_NOF12);

    newtFormAddComponents(form,label,scale,(void *) 0);
  }

  newtLabelSetText(label,text);

  newtScaleSet(scale,percent);

  newtDrawForm(form);

  newtRefresh();

  return true;
}

extern bool ui_window_list(const char *title,const char *text,char **entries,char **entry)
{
  int textbox_width = 0;
  int textbox_height = 0;
  int next_width = 0;
  int next_height = 0;
  int listbox_width = 0;
  int listbox_height = 0;
  newtComponent textbox = 0;
  newtComponent next = 0;
  newtComponent listbox = 0;
  newtComponent form = 0;
  struct newtExitStruct es = {0};
  char **p = 0;

  if(title == 0 || text == 0 || entries == 0 || entry == 0)
  {
    error(strerror(errno));
    return false;
  }

  if(!get_text_screen_size(text,&textbox_width,&textbox_height))
    return false;
  
  if(!get_button_screen_size(NEXT_BUTTON_TEXT,&next_width,&next_height))
    return false;
  
  listbox_width = NEWT_WIDTH;

  listbox_height = NEWT_HEIGHT - textbox_height - next_height - 2;  

  if(newtCenteredWindow(NEWT_WIDTH,NEWT_HEIGHT,title) != 0)
  {
    eprintf("Failed to open a NEWT window.\n");
    return false;
  }
  
  textbox = newtTextbox(0,0,textbox_width,textbox_height,0);

  newtTextboxSetText(textbox,text);

  next = newtButton(NEWT_WIDTH-next_width,NEWT_HEIGHT-next_height,NEXT_BUTTON_TEXT);

  listbox = newtListbox(0,textbox_height+1,listbox_height,NEWT_FLAG_SCROLL);

  newtListboxSetWidth(listbox,listbox_width);

  for( p = entries ; *p != 0 ; ++p )
    newtListboxAppendEntry(listbox,*p,*p);

  newtListboxSetCurrent(listbox,0);

  form = newtForm(0,0,NEWT_FLAG_NOF12);

  newtFormAddComponents(form,textbox,next,listbox,(void *) 0);

  newtFormSetCurrent(form,listbox);

  while(true)
  {
    newtFormRun(form,&es);
    
    if(es.reason == NEWT_EXIT_COMPONENT && es.u.co == next)
    {
      *entry = newtListboxGetCurrent(listbox);
      break;
    }
  }
  
  newtFormDestroy(form);

  newtPopWindow();

  return true;
}

extern void ui_window_text(const char *title,const char *text)
{
  int textbox_width = 0;
  int textbox_height = 0;
  int next_width = 0;
  int next_height = 0;
  newtComponent textbox = 0;
  newtComponent next = 0;
  newtComponent form = 0;
  struct newtExitStruct es = {0};

  if(title == 0 || text == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return;
  }

  if(!get_text_screen_size(text,&textbox_width,&textbox_height))
    return;

  if(!get_button_screen_size(NEXT_BUTTON_TEXT,&next_width,&next_height))
    return;

  if(newtCenteredWindow(NEWT_WIDTH,NEWT_HEIGHT,title) != 0)
  {
    eprintf("Failed to open a NEWT window.\n");
    return;
  }

  textbox = newtTextbox(0,0,textbox_width,textbox_height,0);

  newtTextboxSetText(textbox,text);

  next = newtButton(NEWT_WIDTH-next_width,NEWT_HEIGHT-next_height,NEXT_BUTTON_TEXT);

  form = newtForm(0,0,NEWT_FLAG_NOF12);

  newtFormAddComponents(form,textbox,next,(void *) 0);

  newtFormSetCurrent(form,next);

  while(true)
  {
    newtFormRun(form,&es);

    if(es.reason == NEWT_EXIT_COMPONENT && es.u.co == next)
      break;
  }

  newtFormDestroy(form);

  newtPopWindow();
}

extern bool ui_window_nm(struct nmdevice **devices,struct nmprofile ***profiles)
{
  int textbox_width = 0;
  int textbox_height = 0;
  int listbox_width = 0;
  int listbox_height = 0;
  int next_width = 0;
  int next_height = 0;
  newtComponent textbox = 0;
  newtComponent listbox = 0;
  newtComponent next = 0;
  newtComponent form = 0;
  struct newtExitStruct es = {0};
  bool rv = true;
  const char *options[] =
  {
    "Create Profile",
    "Edit Profile",
    "Delete Profile",
    0
  };
  const uintptr_t CREATE_PROFILE = 0;
  const uintptr_t EDIT_PROFILE = 1;
  const uintptr_t DELETE_PROFILE = 2;

  if(devices == 0 || profiles == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }
  
  if(!get_text_screen_size(NM_CONFIG_TEXT,&textbox_width,&textbox_height))
    return false;

  if(!get_button_screen_size(NEXT_BUTTON_TEXT,&next_width,&next_height))
    return false;

  listbox_width = NEWT_WIDTH;
 
  listbox_height = 3;

  if(newtCenteredWindow(NEWT_WIDTH,NEWT_HEIGHT,NM_CONFIG_TITLE) != 0)
  {
    eprintf("Failed to open a NEWT window.\n");
    return false;
  }

  textbox = newtTextbox(0,0,textbox_width,textbox_height,0);

  newtTextboxSetText(textbox,NM_CONFIG_TEXT);

  listbox = newtListbox(0,textbox_height+1,listbox_height,NEWT_FLAG_RETURNEXIT);

  newtListboxSetWidth(listbox,listbox_width);

  newtListboxAppendEntry(listbox,options[CREATE_PROFILE],(void *) CREATE_PROFILE);

  newtListboxAppendEntry(listbox,options[EDIT_PROFILE],(void *) EDIT_PROFILE);
    
  newtListboxAppendEntry(listbox,options[DELETE_PROFILE],(void *) DELETE_PROFILE);

  next = newtButton(NEWT_WIDTH-next_width,NEWT_HEIGHT-next_height,NEXT_BUTTON_TEXT);

  form = newtForm(0,0,NEWT_FLAG_NOF12);

  newtFormAddComponents(form,textbox,listbox,next,(void *) 0);

  newtFormSetCurrent(form,listbox);

  while(true)
  {
    newtFormRun(form,&es);

    if(es.reason == NEWT_EXIT_COMPONENT && es.u.co == listbox)
    {
      const uintptr_t action = (uintptr_t) newtListboxGetCurrent(listbox);
      char *entries[4096] = {0};
      char *entry = 0;
      char title[TEXT_MAX] = {0};
      char text[TEXT_MAX] = {0};
      size_t i = 0;
      size_t j = 0;
      size_t size = sizeof(entries) / sizeof(*entries);
      struct nmprofile *profile = 0;
      
      if(action == CREATE_PROFILE)
      {
        profile = alloc(struct nmprofile,1);
      
        profile->data = dictionary_new(0);
        
        for( ; profiles[0][i] != 0 ; ++i )
          ;
        
        profiles[0] = redim(profiles[0],struct nmprofile *,i + 2);
        
        profiles[0][i++] = profile;
        
        profiles[0][i] = 0;

        if(!process_nm_profile(profile,devices,profiles[0]))
        {
          rv = false;
          break;
        }
      }
      else if(action == EDIT_PROFILE || action == DELETE_PROFILE)
      {
        for( ; (profile = profiles[0][j]) != 0 ; ++j )
        {
          char *s = 0;
        
          if(profile->data == 0 || i == size - 1)
            continue;
          
          s = iniparser_getstring(profile->data,PROFILE_NAME_KEY,"");
          
          if(strlen(s) == 0)
            continue;
          
          entries[i++] = s;
        }
        
        entries[i] = 0;
        
        if(i == 0)
        {
          ui_dialog_text(NM_NO_PROFILE_TITLE,NM_NO_PROFILE_TEXT);
          continue;
        }
        
        strfcpy(title,sizeof(title),"%s",NM_SELECTION_TITLE);
        
        if(action == EDIT_PROFILE)
          strfcpy(text,sizeof(text),"%s",NM_EDIT_TEXT);
        else if(action == DELETE_PROFILE)
          strfcpy(text,sizeof(text),"%s",NM_DELETE_TEXT);
        
        if(!ui_window_list(title,text,entries,&entry))
        {
          rv = false;
          break;
        }
        
        if(
          (profile = lfind(entry,profiles[0],&j,sizeof(struct nmprofile *),lfind_compare_nm)) == 0 ||
          (profile = *(struct nmprofile **) profile) == 0
        )
        {
          error("could not find the profile");
          rv = false;
          break;
        }
        
        if(action == EDIT_PROFILE && !process_nm_profile(profile,devices,profiles[0]))
        {
          rv = false;
          break;
        }
        else if(action == DELETE_PROFILE)
        {
          if(profile->data != 0)
            iniparser_freedict(profile->data);
            
          profile->data = 0;
        }
      }
    }
    else if(es.reason == NEWT_EXIT_COMPONENT && es.u.co == next)
    {
      break;
    }
  }

  newtFormDestroy(form);

  newtPopWindow();

  return rv;
}

extern bool ui_window_partition(struct device **devices,struct disk **disks)
{
  int textbox_width = 0;
  int textbox_height = 0;
  int next_width = 0;
  int next_height = 0;
  int listbox_width = 0;
  int listbox_height = 0;
  newtComponent textbox = 0;
  newtComponent next = 0;
  newtComponent listbox = 0;
  newtComponent form = 0;
  struct newtExitStruct es = {0};
  int i = 0;
  int j = 0;
  int k = 0;
  char size[10] = {0};
  char text[TEXT_MAX] = {0};

  if(devices == 0 || disks == 0 || sizeof(union partition_action) != sizeof(uintptr_t))
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

  if(!get_text_screen_size(PARTITION_TEXT,&textbox_width,&textbox_height))
    return false;

  if(!get_button_screen_size(NEXT_BUTTON_TEXT,&next_width,&next_height))
    return false;

  listbox_width = NEWT_WIDTH;

  listbox_height = NEWT_HEIGHT - textbox_height - next_height - 2;

  if(newtCenteredWindow(NEWT_WIDTH,NEWT_HEIGHT,PARTITION_TITLE) != 0)
  {
    eprintf("Failed to open a NEWT window.\n");
    return false;
  }

  textbox = newtTextbox(0,0,textbox_width,textbox_height,0);

  newtTextboxSetText(textbox,PARTITION_TEXT);

  next = newtButton(NEWT_WIDTH-next_width,NEWT_HEIGHT-next_height,NEXT_BUTTON_TEXT);

  listbox = newtListbox(0,textbox_height+1,listbox_height,NEWT_FLAG_RETURNEXIT|NEWT_FLAG_SCROLL);

  newtListboxSetWidth(listbox,listbox_width);

  for( ; devices[i] != 0 ; ++i )
  {
    struct device *device = devices[i];
    struct disk *disk = disks[i];
    union partition_action action = {{0}};
    long long freesize = 0;

    size_to_string(size,sizeof(size),device_get_size(device),false);

    strfcpy(text,sizeof(text),"Disk %s: %s label (%s)",device_get_path(device),(disk == 0) ? "nil" : disk_get_type(disk),size);

    action.device_number = i;

    action.disk = true;

    newtListboxAppendEntry(listbox,text,(void *) action.data);

    action.disk = false;

    if(disk != 0)
    {
      action.partition = true;

      for( j = 0, k = disk_partition_get_count(disk) ; j < k ; ++j )
      {
        if(strcmp(disk_partition_get_purpose(disk,j),"extended") == 0)
          continue;

        action.partition_number = j;

        size_to_string(size,sizeof(size),disk_partition_get_size(disk,j),false);

        strfcpy(text,sizeof(text),"%2cPartition %3d: %7s type %8s (%s)",' ',disk_partition_get_number(disk,j),disk_partition_get_purpose(disk,j),(disk_partition_get_active(disk,j)) ? "active" : "inactive",size);

        newtListboxAppendEntry(listbox,text,(void *) action.data);
      }

      action.partition = false;

      action.partition_number = 0;

      if((freesize = disk_get_free_size(disk)) > 0)
      {
        action.space = true;

        size_to_string(size,sizeof(size),freesize,false);

        strfcpy(text,sizeof(text),"%2cFree Space (%s)",' ',size);

        newtListboxAppendEntry(listbox,text,(void *) action.data);

        action.space = false;
      }

      if(k > 0)
      {
        action.delete = true;

        if(strcmp(disk_partition_get_purpose(disk,k-1),"extended") == 0)
          strfcpy(text,sizeof(text),"%2cDelete Extended Partition",' ');
        else
          strfcpy(text,sizeof(text),"%2cDelete Last Partition",' ');

        newtListboxAppendEntry(listbox,text,(void *) action.data);

        action.delete = false;
      }
    }

    action.device_number = 0;

    if(devices[i+1] != 0)
      newtListboxAppendEntry(listbox,"",0);
  }

  newtListboxSetCurrent(listbox,0);

  form = newtForm(0,0,NEWT_FLAG_NOF12);

  newtFormAddComponents(form,textbox,next,listbox,(void *) 0);

  newtFormSetCurrent(form,listbox);

  while(true)
  {
    newtFormRun(form,&es);

    if(es.reason == NEWT_EXIT_COMPONENT && es.u.co == listbox)
    {
      const union partition_action action = { .data = (uintptr_t) newtListboxGetCurrent(listbox) };
      union partition_action key = { .data = action.data };
      struct device *device = devices[action.device_number];
      struct disk *disk = disks[action.device_number];

      if(action.disk)
      {
        if(ui_dialog_partition_new_table(device,&disk))
        {
          disks[action.device_number] = disk;

          key.disk = false;

          key.partition = true;

          for( i = 0 ; i < 255 ; ++i )
          {
            key.partition_number = i;

            newtListboxDeleteEntry(listbox,(void *) key.data);
          }

          key.data = action.data;

          key.disk = false;

          key.space = true;

          newtListboxDeleteEntry(listbox,(void *) key.data);

          key.data = action.data;

          key.disk = false;

          key.delete = true;

          newtListboxDeleteEntry(listbox,(void *) key.data);

          key.data = action.data;

          size_to_string(size,sizeof(size),device_get_size(device),false);

          strfcpy(text,sizeof(text),"Disk %s: %s label (%s)",device_get_path(device),disk_get_type(disk),size);

          newtListboxInsertEntry(listbox,text,(void *) key.data,(void *) key.data);

          newtListboxDeleteEntry(listbox,(void *) key.data);

          key.disk = false;

          key.space = true;

          size_to_string(size,sizeof(size),disk_get_free_size(disk),false);

          strfcpy(text,sizeof(text),"%2cFree Space (%s)",' ',size);

          newtListboxInsertEntry(listbox,text,(void *) key.data,(void *) action.data);

          newtListboxSetCurrentByKey(listbox,(void *) action.data);
        }
      }
      else if(action.partition)
      {
        unsigned char partition = action.partition_number;

        if(ui_dialog_partition_modify_partition(disk,partition))
        {
          size_to_string(size,sizeof(size),disk_partition_get_size(disk,partition),false);

          strfcpy(text,sizeof(text),"%2cPartition %3d: %7s type %8s (%s)",' ',disk_partition_get_number(disk,partition),disk_partition_get_purpose(disk,partition),(disk_partition_get_active(disk,partition)) ? "active" : "inactive",size);

          newtListboxInsertEntry(listbox,text,(void *) key.data,(void *) key.data);

          newtListboxDeleteEntry(listbox,(void *) key.data);

          newtListboxSetCurrentByKey(listbox,(void *) action.data);
        }
      }
      else if(action.space)
      {
        unsigned char partition = disk_partition_get_count(disk);

        if(ui_dialog_partition_new_partition(disk))
        {
          key.space = false;

          key.delete = true;

          if(strcmp(disk_partition_get_purpose(disk,partition),"extended") != 0)
            strfcpy(text,sizeof(text),"%2cDelete Last Partition",' ');
          else
            strfcpy(text,sizeof(text),"%2cDelete Extended Partition",' ');

          newtListboxDeleteEntry(listbox,(void *) key.data);

          newtListboxInsertEntry(listbox,text,(void *) key.data,(void *) action.data);

          key.data = action.data;

          if(disk_get_free_size(disk) > 0)
          {
            size_to_string(size,sizeof(size),disk_get_free_size(disk),false);

            strfcpy(text,sizeof(text),"%2cFree Space (%s)",' ',size);

            newtListboxInsertEntry(listbox,text,(void *) key.data,(void *) action.data);
          }

          if(strcmp(disk_partition_get_purpose(disk,partition),"extended") != 0)
          {
            key.space = false;

            key.partition = true;

            key.partition_number = partition;

            size_to_string(size,sizeof(size),disk_partition_get_size(disk,partition),false);

            strfcpy(text,sizeof(text),"%2cPartition %3d: %7s type %8s (%s)",' ',disk_partition_get_number(disk,partition),disk_partition_get_purpose(disk,partition),(disk_partition_get_active(disk,partition)) ? "active" : "inactive",size);

            newtListboxInsertEntry(listbox,text,(void *) key.data,(void *) action.data);
          }

          newtListboxDeleteEntry(listbox,(void *) action.data);

          newtListboxSetCurrentByKey(listbox,(void *) ((disk_get_free_size(disk) > 0) ? action.data : key.data));
        }
      }
      else if(action.delete)
      {
        unsigned char partition = disk_partition_get_count(disk) - 1;

        if(strcmp(disk_partition_get_purpose(disk,partition),"extended") != 0)
        {
          key.delete = false;

          key.partition = true;

          key.partition_number = partition;

          newtListboxDeleteEntry(listbox,(void *) key.data);

          key.data = action.data;
        }

        disk_delete_partition(disk);

        if(partition > 0)
        {
          if(strcmp(disk_partition_get_purpose(disk,partition-1),"extended") != 0)
            strfcpy(text,sizeof(text),"%2cDelete Last Partition",' ');
          else
            strfcpy(text,sizeof(text),"%2cDelete Extended Partition",' ');

          newtListboxInsertEntry(listbox,text,(void *) key.data,(void *) action.data);
        }

        key.delete = false;

        key.space = true;

        size_to_string(size,sizeof(size),disk_get_free_size(disk),false);

        strfcpy(text,sizeof(text),"%2cFree Space (%s)",' ',size);

        newtListboxDeleteEntry(listbox,(void *) key.data);

        newtListboxInsertEntry(listbox,text,(void *) key.data,(void *) action.data);

        newtListboxDeleteEntry(listbox,(void *) action.data);

        newtListboxSetCurrentByKey(listbox,(void *) ((partition > 0) ? action.data : key.data));
      }
    }
    else if(es.reason == NEWT_EXIT_COMPONENT && es.u.co == next)
    {
      bool empty = true;
      bool bios = true;
      
      for( i = 0 ; devices[i] != 0 ; ++i )
      {
        struct disk *disk = disks[i];
        
        if(disk == 0 || (k = disk_partition_get_count(disk)) == 0)
          continue;
        
        empty = false;
        
        if(bios)
          bios = disk_can_store_bios_grub(disk);
      }
    
      if(empty)
      {
        ui_dialog_text(NO_PARTITION_TITLE,NO_PARTITION_TEXT);
        continue;
      }
    
      if(!bios && !ui_dialog_yesno(NO_GRUB_BIOS_TITLE,NO_GRUB_BIOS_TEXT,true))
        continue;
    
      break;
    }
  }

  newtFormDestroy(form);

  newtPopWindow();

  return true;
}

extern bool ui_window_raid(struct device ***unused,struct raid ***used,struct raid ***stop)
{
  int textbox_width = 0;
  int textbox_height = 0;
  int next_width = 0;
  int next_height = 0;
  int listbox_width = 0;
  int listbox_height = 0;
  newtComponent textbox = 0;
  newtComponent next = 0;
  newtComponent listbox = 0;
  newtComponent form = 0;
  struct newtExitStruct es = {0};
  char size[10] = {0};
  char text[TEXT_MAX] = {0};

  if(unused == 0 || used == 0 || stop == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }
  
  if(!get_text_screen_size(RAID_TEXT,&textbox_width,&textbox_height))
    return false;

  if(!get_button_screen_size(NEXT_BUTTON_TEXT,&next_width,&next_height))
    return false;

  listbox_width = NEWT_WIDTH;

  listbox_height = NEWT_HEIGHT - textbox_height - next_height - 2;

  if(newtCenteredWindow(NEWT_WIDTH,NEWT_HEIGHT,RAID_TITLE) != 0)
  {
    eprintf("Failed to open a NEWT window.\n");
    return false;
  }

  textbox = newtTextbox(0,0,textbox_width,textbox_height,0);

  newtTextboxSetText(textbox,RAID_TEXT);

  next = newtButton(NEWT_WIDTH-next_width,NEWT_HEIGHT-next_height,NEXT_BUTTON_TEXT);

  listbox = newtListbox(0,textbox_height+1,listbox_height,NEWT_FLAG_RETURNEXIT|NEWT_FLAG_SCROLL);

  newtListboxSetWidth(listbox,listbox_width);

  for( int i = 0 ; used[0][i] != 0 ; ++i )
  {
    struct raid *raid = used[0][i];
  
    size_to_string(size,sizeof(size),raid_get_size(raid),false);
  
    strfcpy(text,sizeof(text),"%s %s level%d",raid_get_path(raid),size,raid_get_level(raid));
    
    newtListboxAppendEntry(listbox,text,raid);
  }

  if(unused[0][0] != 0 && unused[0][1] != 0)
    newtListboxAppendEntry(listbox,RAID_CREATE_TEXT,0);

  newtListboxSetCurrent(listbox,0);

  form = newtForm(0,0,NEWT_FLAG_NOF12);

  newtFormAddComponents(form,textbox,next,listbox,(void *) 0);

  newtFormSetCurrent(form,listbox);

  while(true)
  {
    newtFormRun(form,&es);

    if(es.reason == NEWT_EXIT_COMPONENT && es.u.co == listbox)
    {
      struct raid *raid = newtListboxGetCurrent(listbox);
      
      if(raid == 0)
      {
        if(ui_dialog_raid(unused,used,&raid))
        {
          size_to_string(size,sizeof(size),raid_get_size(raid),false);
  
          strfcpy(text,sizeof(text),"%s %s level%d",raid_get_path(raid),size,raid_get_level(raid));
                  
          newtListboxDeleteEntry(listbox,0);

          newtListboxAppendEntry(listbox,text,raid);
          
          if(unused[0][0] != 0 && unused[0][1] != 0)
            newtListboxAppendEntry(listbox,RAID_CREATE_TEXT,0);
          
          newtListboxSetCurrentByKey(listbox,raid);
        }
      }
      else
      {
        const char *origin = raid_get_origin(raid);
      
        update_raid_remove(unused,used,raid);
        
        newtListboxDeleteEntry(listbox,raid);

        newtListboxDeleteEntry(listbox,0);
        
        if(unused[0][0] != 0 && unused[0][1] != 0)
        {
          newtListboxAppendEntry(listbox,RAID_CREATE_TEXT,0);
          newtListboxSetCurrentByKey(listbox,0);
        }
        else
          newtListboxSetCurrent(listbox,0);
        
        if(strcmp(origin,"memory") == 0)
          raid_close(raid,false,true);
        else if(strcmp(origin,"device") == 0)
          update_raid_stop_add(stop,raid);
      }
    }
    else if(es.reason == NEWT_EXIT_COMPONENT && es.u.co == next)
    {
      break;
    }
  }

  newtFormDestroy(form);

  newtPopWindow();

  return true;
}

extern bool ui_window_format(struct format **targets)
{
  int textbox_width = 0;
  int textbox_height = 0;
  int next_width = 0;
  int next_height = 0;
  int listbox_width = 0;
  int listbox_height = 0;
  newtComponent textbox = 0;
  newtComponent next = 0;
  newtComponent listbox = 0;
  struct format **p = 0;
  char text[TEXT_MAX] = {0};
  newtComponent form = 0;
  struct newtExitStruct es = {0};

  if(targets == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

  if(!get_text_screen_size(FORMAT_TEXT,&textbox_width,&textbox_height))
    return false;

  if(!get_button_screen_size(NEXT_BUTTON_TEXT,&next_width,&next_height))
    return false;

  listbox_width = NEWT_WIDTH;

  listbox_height = NEWT_HEIGHT - textbox_height - next_height - 2;

  if(newtCenteredWindow(NEWT_WIDTH,NEWT_HEIGHT,FORMAT_TITLE) != 0)
  {
    eprintf("Failed to open a NEWT window.\n");
    return false;
  }

  textbox = newtTextbox(0,0,textbox_width,textbox_height,0);

  newtTextboxSetText(textbox,FORMAT_TEXT);

  next = newtButton(NEWT_WIDTH-next_width,NEWT_HEIGHT-next_height,NEXT_BUTTON_TEXT);

  listbox = newtListbox(0,textbox_height+1,listbox_height,NEWT_FLAG_RETURNEXIT|NEWT_FLAG_SCROLL);

  newtListboxSetWidth(listbox,listbox_width);

  for( p = targets ; *p != 0 ; ++p )
  {
    struct format *target = *p;

    strfcpy(text,sizeof(text),"%-11s %-11s %-11s",target->devicepath,target->size,target->filesystem);

    newtListboxAppendEntry(listbox,text,target);
  }

  newtListboxSetCurrentByKey(listbox,targets[0]);

  form = newtForm(0,0,NEWT_FLAG_NOF12);

  newtFormAddComponents(form,textbox,next,listbox,(void *) 0);

  newtFormSetCurrent(form,listbox);

  while(true)
  {
    newtFormRun(form,&es);

    if(es.reason == NEWT_EXIT_COMPONENT && es.u.co == listbox)
    {
      struct format *target = newtListboxGetCurrent(listbox);

      ui_dialog_format(targets,target);

      if(target->newfilesystem != 0 && target->mountpath != 0)
      {
        strfcpy(text,sizeof(text),"%-11s %-11s %-11s %-11s",target->devicepath,target->size,target->newfilesystem,(strcmp(target->newfilesystem,"swap") == 0) ? "active" : target->mountpath);

        newtListboxInsertEntry(listbox,text,target,target);

        newtListboxDeleteEntry(listbox,target);

        newtListboxSetCurrentByKey(listbox,target);
      }

      continue;
    }
    else if(es.reason == NEWT_EXIT_COMPONENT && es.u.co == next)
    {
      bool swap = false;
      bool root = false;

      for( p = targets ; *p != 0 ; ++p )
      {
        struct format *target = *p;

        if(target->newfilesystem == 0 && target->mountpath == 0)
          continue;

        if(strcmp(target->newfilesystem,"swap") == 0)
        {
          swap = true;
          continue;
        }

        if(strcmp(target->mountpath,g->hostroot) == 0)
        {
          root = true;
          continue;
        }
      }

      if(!swap && !ui_dialog_yesno(NO_SWAP_TITLE,NO_SWAP_TEXT,true))
        continue;

      if(!root)
      {
        ui_dialog_text(NO_ROOT_TITLE,NO_ROOT_TEXT);
        continue;
      }

      break;
    }
  }

  newtFormDestroy(form);

  newtPopWindow();

  return true;
}

extern bool ui_window_host(char **hostname,char **prettyhostname)
{
  int textbox_width = 0;
  int textbox_height = 0;
  int label1_width = 0;
  int label1_height = 0;
  int label2_width = 0;
  int label2_height = 0;
  int entry_left = 0;
  int entry_width = 0;
  int next_width = 0;
  int next_height = 0;
  newtComponent textbox = 0;
  newtComponent label1 = 0;
  newtComponent entry1 = 0;
  const char *name1 = 0;
  newtComponent label2 = 0;
  newtComponent entry2 = 0;
  const char *name2 = 0;
  newtComponent next = 0;
  newtComponent form = 0;
  struct newtExitStruct es = {0};

  if(hostname == 0 || prettyhostname == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }
  
  if(!get_text_screen_size(HOST_TEXT,&textbox_width,&textbox_height))
    return false;
  
  if(!get_label_screen_size(HOSTNAME_ENTRY_TEXT,&label1_width,&label1_height))
    return false;
  
  if(!get_label_screen_size(PRETTY_HOSTNAME_ENTRY_TEXT,&label2_width,&label2_height))
    return false;

  entry_left = max(label1_width,label2_width) + 1;

  entry_width = NEWT_WIDTH - entry_left;

  if(!get_button_screen_size(NEXT_BUTTON_TEXT,&next_width,&next_height))
    return false;

  if(newtCenteredWindow(NEWT_WIDTH,NEWT_HEIGHT,HOST_TITLE) != 0)
  {
    eprintf("Failed to open a NEWT window.\n");
    return false;
  }

  textbox = newtTextbox(0,0,textbox_width,textbox_height,0);

  newtTextboxSetText(textbox,HOST_TEXT);

  label1 = newtLabel(0,textbox_height+1,HOSTNAME_ENTRY_TEXT);

  entry1 = newtEntry(entry_left,textbox_height+1,"",entry_width,&name1,NEWT_FLAG_SCROLL);

  label2 = newtLabel(0,textbox_height+label1_height+2,PRETTY_HOSTNAME_ENTRY_TEXT);

  entry2 = newtEntry(entry_left,textbox_height+label1_height+2,"",entry_width,&name2,NEWT_FLAG_SCROLL);

  next = newtButton(NEWT_WIDTH-next_width,NEWT_HEIGHT-next_height,NEXT_BUTTON_TEXT);

  form = newtForm(0,0,NEWT_FLAG_NOF12);

  newtFormAddComponents(form,textbox,label1,entry1,label2,entry2,next,(void *) 0);

  newtFormSetCurrent(form,entry1);

  while(true)
  {
    newtFormRun(form,&es);

    if(es.reason == NEWT_EXIT_COMPONENT && es.u.co == next)
    {
      if(strlen(name1) == 0 || strlen(name2) == 0 || !is_dns_label(name1))
      {
        ui_dialog_text(HOST_ERROR_TITLE,HOST_ERROR_TEXT);
        continue;
      }

      *hostname = strdup(name1);
      
      *prettyhostname = strdup(name2);

      break;
    }
  }

  newtFormDestroy(form);

  newtPopWindow();
  
  return true;
}

extern bool ui_window_root(struct account *data)
{
  int textbox_width = 0;
  int textbox_height = 0;
  int label1_width = 0;
  int label1_height = 0;
  int label2_width = 0;
  int label2_height = 0;
  int entry_left = 0;
  int entry_width = 0;
  int next_width = 0;
  int next_height = 0;
  newtComponent textbox = 0;
  newtComponent label1 = 0;
  newtComponent entry1 = 0;
  const char *password1 = 0;
  newtComponent label2 = 0;
  newtComponent entry2 = 0;
  const char *password2 = 0;
  newtComponent next = 0;
  newtComponent form = 0;
  struct newtExitStruct es = {0};

  if(data == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

  if(!get_text_screen_size(ROOT_TEXT,&textbox_width,&textbox_height))
    return false;

  if(!get_label_screen_size(PASSWORD_ENTER_TEXT,&label1_width,&label1_height))
    return false;

  if(!get_label_screen_size(PASSWORD_CONFIRM_TEXT,&label2_width,&label2_height))
    return false;

  entry_left = max(label1_width,label2_width) + 1;

  entry_width = NEWT_WIDTH - entry_left;

  if(!get_button_screen_size(NEXT_BUTTON_TEXT,&next_width,&next_height))
    return false;

  if(newtCenteredWindow(NEWT_WIDTH,NEWT_HEIGHT,ROOT_TITLE) != 0)
  {
    eprintf("Failed to open a NEWT window.\n");
    return false;
  }

  textbox = newtTextbox(0,0,textbox_width,textbox_height,0);

  newtTextboxSetText(textbox,ROOT_TEXT);

  label1 = newtLabel(0,textbox_height+1,PASSWORD_ENTER_TEXT);

  entry1 = newtEntry(entry_left,textbox_height+1,"",entry_width,&password1,NEWT_FLAG_PASSWORD|NEWT_FLAG_SCROLL);

  label2 = newtLabel(0,textbox_height+label1_height+2,PASSWORD_CONFIRM_TEXT);

  entry2 = newtEntry(entry_left,textbox_height+label1_height+2,"",entry_width,&password2,NEWT_FLAG_PASSWORD|NEWT_FLAG_SCROLL);

  next = newtButton(NEWT_WIDTH-next_width,NEWT_HEIGHT-next_height,NEXT_BUTTON_TEXT);

  form = newtForm(0,0,NEWT_FLAG_NOF12);

  newtFormAddComponents(form,textbox,label1,entry1,label2,entry2,next,(void *) 0);

  newtFormSetCurrent(form,entry1);

  while(true)
  {
    newtFormRun(form,&es);

    if(es.reason == NEWT_EXIT_COMPONENT && es.u.co == next)
    {
      if(
        strlen(password1) == 0           ||
        strlen(password2) == 0           ||
        strcmp(password1,password2) != 0
      )
      {
        ui_dialog_text(USER_ERROR_TITLE,USER_ERROR_TEXT);
        continue;
      }

      break;
    }
  }

  data->name = 0;

  data->user = strdup("root");

  data->password = strdup(password1);

  data->group = strdup("root");

  data->groups = 0;

  data->home = strdup("/root");

  data->shell = strdup("/bin/bash");

  newtFormDestroy(form);

  newtPopWindow();

  return true;
}

extern bool ui_window_user(struct account *data)
{
  int textbox_width = 0;
  int textbox_height = 0;
  int label1_width = 0;
  int label1_height = 0;
  int label2_width = 0;
  int label2_height = 0;
  int label3_width = 0;
  int label3_height = 0;
  int label4_width = 0;
  int label4_height = 0;
  int entry_left = 0;
  int entry_width = 0;
  int next_width = 0;
  int next_height = 0;
  newtComponent textbox = 0;
  newtComponent label1 = 0;
  newtComponent entry1 = 0;
  const char *name = 0;
  newtComponent label2 = 0;
  newtComponent entry2 = 0;
  const char *user = 0;
  newtComponent label3 = 0;
  newtComponent entry3 = 0;
  const char *password1 = 0;
  newtComponent label4 = 0;
  newtComponent entry4 = 0;
  const char *password2 = 0;
  newtComponent next = 0;
  newtComponent form = 0;
  struct newtExitStruct es = {0};
  char home[PATH_MAX] = {0};

  if(data == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

  if(!get_text_screen_size(USER_TEXT,&textbox_width,&textbox_height))
    return false;

  if(!get_label_screen_size(NAME_ENTRY_TEXT,&label1_width,&label1_height))
    return false;

  if(!get_label_screen_size(USER_ENTRY_TEXT,&label2_width,&label2_height))
    return false;

  if(!get_label_screen_size(PASSWORD_ENTER_TEXT,&label3_width,&label3_height))
    return false;

  if(!get_label_screen_size(PASSWORD_CONFIRM_TEXT,&label4_width,&label4_height))
    return false;

  entry_left = maxv( (long long []) { label1_width, label2_width, label3_width, label4_width }, 4) + 1;

  entry_width = NEWT_WIDTH - entry_left;

  if(!get_button_screen_size(NEXT_BUTTON_TEXT,&next_width,&next_height))
    return false;

  if(newtCenteredWindow(NEWT_WIDTH,NEWT_HEIGHT,USER_TITLE) != 0)
  {
    eprintf("Failed to open a NEWT window.\n");
    return false;
  }

  textbox = newtTextbox(0,0,textbox_width,textbox_height,0);

  newtTextboxSetText(textbox,USER_TEXT);

  label1 = newtLabel(0,textbox_height+1,NAME_ENTRY_TEXT);

  entry1 = newtEntry(entry_left,textbox_height+1,"",entry_width,&name,NEWT_FLAG_SCROLL);

  label2 = newtLabel(0,textbox_height+label1_height+2,USER_ENTRY_TEXT);

  entry2 = newtEntry(entry_left,textbox_height+label1_height+2,"",entry_width,&user,NEWT_FLAG_SCROLL);

  label3 = newtLabel(0,textbox_height+label1_height+label2_height+3,PASSWORD_ENTER_TEXT);

  entry3 = newtEntry(entry_left,textbox_height+label1_height+label2_height+3,"",entry_width,&password1,NEWT_FLAG_PASSWORD|NEWT_FLAG_SCROLL);

  label4 = newtLabel(0,textbox_height+label1_height+label2_height+label3_height+4,PASSWORD_CONFIRM_TEXT);

  entry4 = newtEntry(entry_left,textbox_height+label1_height+label2_height+label3_height+4,"",entry_width,&password2,NEWT_FLAG_PASSWORD|NEWT_FLAG_SCROLL);

  next = newtButton(NEWT_WIDTH-next_width,NEWT_HEIGHT-next_height,NEXT_BUTTON_TEXT);

  form = newtForm(0,0,NEWT_FLAG_NOF12);

  newtFormAddComponents(form,textbox,label1,entry1,label2,entry2,label3,entry3,label4,entry4,next,(void *) 0);

  newtFormSetCurrent(form,entry1);

  while(true)
  {
    newtFormRun(form,&es);

    if(es.reason == NEWT_EXIT_COMPONENT && es.u.co == next)
    {
      if(
        !is_user_name(user)              ||
        strlen(password1) == 0           ||
        strlen(password2) == 0           ||
        strcmp(password1,password2) != 0
      )
      {
        ui_dialog_text(USER_ERROR_TITLE,USER_ERROR_TEXT);
        continue;
      }

      break;
    }
  }

  data->name = (get_text_length(name) > 0) ? strdup(name) : 0;

  data->user = strdup(user);

  data->password = strdup(password1);

  data->group = strdup("users");

  data->groups = strdup("audio,camera,cdrom,floppy,scanner,video,uucp,storage,netdev,locate");

  strfcpy(home,sizeof(home),"/home/%s",user);

  data->home = strdup(home);

  data->shell = strdup("/bin/bash");

  newtFormDestroy(form);

  newtPopWindow();

  return true;
}

extern bool ui_window_time(char **data,char **zone,bool *utc)
{
  int textbox_width = 0;
  int textbox_height = 0;
  int next_width = 0;
  int next_height = 0;
  int checkbox_width = 0;
  int checkbox_height = 0;
  int listbox_width = 0;
  int listbox_height = 0;
  newtComponent textbox = 0;
  newtComponent next = 0;
  newtComponent checkbox = 0;
  char result = '*';
  newtComponent listbox = 0;
  newtComponent form = 0;
  struct newtExitStruct es = {0};

  if(data == 0 || zone == 0 || utc == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

  if(!get_text_screen_size(TIME_TEXT,&textbox_width,&textbox_height))
    return false;

  if(!get_button_screen_size(NEXT_BUTTON_TEXT,&next_width,&next_height))
    return false;

  if(!get_checkbox_screen_size(UTC_TEXT,&checkbox_width,&checkbox_height))
    return false;

  listbox_width = NEWT_WIDTH;

  listbox_height = NEWT_HEIGHT - textbox_height - next_height - checkbox_height - 3;

  if(newtCenteredWindow(NEWT_WIDTH,NEWT_HEIGHT,TIME_TITLE) != 0)
  {
    eprintf("Failed to open a NEWT window.\n");
    return false;
  }

  textbox = newtTextbox(0,0,textbox_width,textbox_height,0);

  newtTextboxSetText(textbox,TIME_TEXT);

  next = newtButton(NEWT_WIDTH-next_width,NEWT_HEIGHT-next_height,NEXT_BUTTON_TEXT);

  checkbox = newtCheckbox(0,textbox_height+1,UTC_TEXT,result,0,&result);

  listbox = newtListbox(0,textbox_height+checkbox_height+2,listbox_height,NEWT_FLAG_SCROLL);

  newtListboxSetWidth(listbox,listbox_width);

  for( char **p = data ; *p != 0 ; ++p )
    newtListboxAppendEntry(listbox,*p,*p);

  newtListboxSetCurrent(listbox,0);

  form = newtForm(0,0,NEWT_FLAG_NOF12);

  newtFormAddComponents(form,textbox,next,checkbox,listbox,(void *) 0);

  newtFormSetCurrent(form,listbox);

  while(true)
  {
    newtFormRun(form,&es);

    if(es.reason == NEWT_EXIT_COMPONENT && es.u.co == next)
      break;
  }

  *zone = newtListboxGetCurrent(listbox);

  *utc = (result == '*');

  newtFormDestroy(form);

  newtPopWindow();

  return true;
}

extern bool ui_window_install(struct install *data)
{
  int textbox_width = 0;
  int textbox_height = 0;
  int next_width = 0;
  int next_height = 0;
  int checkboxtree_width = 0;
  int checkboxtree_height = 0;
  newtComponent textbox = 0;
  newtComponent next = 0;
  newtComponent checkboxtree = 0;
  int i = 0;
  struct install *grp = 0;
  newtComponent form = 0;
  struct newtExitStruct es = {0};
  bool result = true;

  if(data == 0)
  {
    errno = EINVAL;
    error(strerror(errno));
    return false;
  }

  if(!get_text_screen_size(INSTALL_TEXT,&textbox_width,&textbox_height))
    return false;

  if(!get_button_screen_size(NEXT_BUTTON_TEXT,&next_width,&next_height))
    return false;

  checkboxtree_width = NEWT_WIDTH;

  checkboxtree_height = NEWT_HEIGHT - textbox_height - next_height - 2;

  if(newtCenteredWindow(NEWT_WIDTH,NEWT_HEIGHT,INSTALL_TITLE) != 0)
  {
    eprintf("Failed to open a NEWT window.\n");
    return false;
  }

  textbox = newtTextbox(0,0,textbox_width,textbox_height,0);

  newtTextboxSetText(textbox,INSTALL_TEXT);

  next = newtButton(NEWT_WIDTH-next_width,NEWT_HEIGHT-next_height,NEXT_BUTTON_TEXT);

  checkboxtree = newtCheckboxTree(0,textbox_height+1,checkboxtree_height,NEWT_FLAG_SCROLL);

  newtCheckboxTreeSetWidth(checkboxtree,checkboxtree_width);

  grp = data;

  while(grp->name != 0)
  {
    newtCheckboxTreeAddItem(checkboxtree,grp->name,&grp->checked,0,i,NEWT_ARG_LAST);
    newtCheckboxTreeSetEntryValue(checkboxtree,&grp->checked,(grp->checked) ? '*' : ' ');
    ++i;
    ++grp;
  }

  form = newtForm(0,0,NEWT_FLAG_NOF12);

  newtFormAddComponents(form,textbox,next,checkboxtree,(void *) 0);

  newtFormSetCurrent(form,checkboxtree);

  while(true)
  {
    newtFormRun(form,&es);

    if(es.reason == NEWT_EXIT_COMPONENT && es.u.co == next)
    {
      grp = data;

      while(grp->name != 0)
      {
        if(strcmp(grp->name,"base") == 0)
          break;
        ++grp;
      }

      if(grp != 0 && newtCheckboxTreeGetEntryValue(checkboxtree,&grp->checked) != '*')
      {
        ui_dialog_text(NO_BASE_TITLE,NO_BASE_TEXT);
        continue;
      }

      result = true;

      break;
    }
  }

  grp = data;

  while(grp->name != 0)
  {
    grp->checked = (newtCheckboxTreeGetEntryValue(checkboxtree,&grp->checked) == '*');
    ++grp;
  }

  newtFormDestroy(form);

  newtPopWindow();

  return result;
}
