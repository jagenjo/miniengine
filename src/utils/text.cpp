#include "text.h"

#include "../includes.h"

#include <sys/stat.h>
#include <string>
#include <algorithm>

//extern HWND GLOBmainwindow;
char g_string_temporal[256];

text::text()
: data(NULL)
{}

text::text(const char *name)
: data(NULL)
{
  FILE *f;
  struct stat stbuffer;
  
  stat(name,&stbuffer);
  f = fopen(name,"rb");
  size = stbuffer.st_size;
  data = new char[size];
  sl = 0;
  fread(data,size,1,f);
  fclose(f);
}

text::~text()
{
  if (data!=NULL) 
    delete data;
}

bool text::create(const char *name)
{
  FILE *f;
  struct stat stbuffer;
  
  stat(name,&stbuffer);
  f = fopen(name,"rb");
  if (f == NULL)
    {
      char s[256];
      sprintf(s,"The resource file %s does not exist\n",name);
      //MessageBox(GLOBmainwindow,s,"SR",MB_OK);
      return false;
    }
  size = stbuffer.st_size;
  data = new char[size];
  sl = 0;
  fread(data,size,1,f);
  fclose(f);
	return true;
}

int legal(char c)
{
  int res;
  
  res = (c>32);
  return res;
}

char *text::getword()
{
  int p0,p1,i;

  p0 = sl;
  if (p0 >= size)
    return NULL;
  while (!legal(data[p0]) && p0<size)
    p0++;
  if (p0 >= size)
    return NULL;
  p1 = p0+1;
  while (legal(data[p1]))
    p1++;
  
  for (i=p0;i<p1;i++)
    {
      if ((data[i]<='z') && (data[i]>='a'))
	data[i] += ('A'-'a');
      g_string_temporal[i-p0] = data[i];
    }
  g_string_temporal[p1-p0] = '\0';
  sl = p1;
	std::string s(g_string_temporal);
	std::transform(s.begin(),s.end(),s.begin(),toupper);
  //strupr(g_string_temporal);
	strcpy(g_string_temporal,s.c_str());
  return g_string_temporal;
}


char *text::getcommaword()
{
  int p0,p1,i;

  p0 = sl;
  while (data[p0]!='"')
    p0++;
  p0++;
  p1 = p0+1;
  while (data[p1]!='"')
    p1++;
  for (i=p0;i<p1;i++)
    {
      //if ((data[i]<='z') && (data[i]>='a')) data[i]+=('A'-'a');
      g_string_temporal[i-p0] = data[i];
    }
  g_string_temporal[p1-p0] = '\0';
  sl = p1+1;
  return g_string_temporal;
}

int text::getint()
{
  return( atoi(getword()) );
}

double text::getfloat()
{
  return( atof(getword()) );
}

void text::goback()
{
  int p0,p1;

  p0=sl;
  while (!legal(data[p0])) p0--;
  p1=p0-1;
  while (legal(data[p1])) p1--;
  sl=p1;
}


int text::countchar(char c)
{
  int res;
  unsigned int i;
  
  res=0;
  for (i=0;i<size;i++)
    if (data[i]==c) res++;
  return res;
}


void text::reset()
{  
  sl=0;
}

void text::destroy()
{
  if (data!=NULL) 
    delete data;
}


int text::countword(char *s)
{
  int res;
  unsigned int i;
  int final;
  unsigned int si;

  res=0;
  final=0;
  i=0;
  while (!final)
    {
      si=0;
      while (toupper(data[i])==toupper(s[si]))
	{
	  i++;
	  si++;
	}
      res+=(si==strlen(s));
      i+=si;
      i++;
      final=(i>=size);
    }
  return res;
}


int text::countwordfromhere(char *s)
{
  int res;
  unsigned int i;
  int final;
  unsigned int si;
  
  res=0;
  final=0;
  i=sl;
  while (!final)
    {
      si=0;
      while (toupper(data[i])==toupper(s[si]))
	{
	  i++;
	  si++;
	}
      res+=(si==strlen(s));
      i+=si;
      i++;
      final=(i>=size);
    }
  return res;
}

int text::eof()
{  
  return (sl>size);
}


void text::seek(const char *token)
{
  char *dummy=getword();

  while (strcmp(dummy,token) && (sl<size))
    dummy = getword();  
}

int text::CountObjs()
{
	char *dummy=getword();
	int objCount = 0;

	while (sl < size)
	{
		if( strcmp(dummy, "*GEOMOBJECT") == 0 ) objCount++;
		dummy = getword();
	}

	return objCount;
}
