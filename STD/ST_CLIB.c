/****************************************************************************
 *
 *  SiriusRTOS
 *  ST_CLIB.c - Standard C Library Abstraction
 *  Version 1.00
 *
 *  Copyright 2010 by SpaceShadow
 *  All rights reserved!
 *
 ***************************************************************************/


/****************************************************************************
 *
 *  Includes
 *
 ***************************************************************************/

#include "ST_API.h"

#if (ST_USE_MALLOC_CLIB)
  #include <stdlib.h>
#endif


/****************************************************************************
 *
 *  Global variables
 *
 ***************************************************************************/

#if !(ST_USE_MALLOC_CLIB)

  /* Static memory pool for internal allocation */
  #if (ST_MALLOC_BUFFERED)
    UINT8 stMemoryPool[ST_MALLOC_SIZE];
  #endif

#endif


/****************************************************************************
 *
 *  Memory Allocation Wrapper
 *
 ***************************************************************************/


/***************************************************************************/
#if (ST_USE_MALLOC_CLIB)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    stMemAlloc
 *
 *  Description:
 *    Allocates a memory block of the specified size.
 *
 *  Parameters:
 *    Size - Size of the memory to be allocated in bytes.
 *
 *  Return:
 *    Pointer to the newly allocated block, or NULL on failure.
 *
 ***************************************************************************/

PVOID stMemAlloc(SIZE Size)
{
  PVOID NewMem;

  /* Attempt to allocate memory */
  NewMem = malloc(Size);
  if(!NewMem)
    stSetLastError(ERR_NOT_ENOUGH_MEMORY);

  /* Return the pointer */
  return NewMem;
}


/****************************************************************************
 *
 *  Name:
 *    stMemFree
 *
 *  Description:
 *    Frees a previously allocated memory block.
 *
 *  Parameters:
 *    Ptr - Pointer to the memory block to be released.
 *
 *  Return:
 *    TRUE on success, FALSE on failure.
 *
 ***************************************************************************/

BOOL stMemFree(PVOID Ptr)
{
  /* Release the memory */
  free(Ptr);
  return TRUE;
}


/***************************************************************************/
#endif /* ST_USE_MALLOC_CLIB */
/***************************************************************************/


/****************************************************************************
 *
 *  Memory Block Operations
 *
 ***************************************************************************/


/***************************************************************************/
#if !(ST_USE_MEMORY_CLIB)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    stMemCmp
 *
 *  Description:
 *    Compares the first n bytes of two memory areas.
 *
 *  Parameters:
 *    s1 - Pointer to the first memory block.
 *    s2 - Pointer to the second memory block.
 *    n - Number of bytes to compare.
 *
 *  Return:
 *    - < 0 if s1 is less than s2
 *    - 0 if s1 is equal to s2
 *    - > 0 if s1 is greater than s2
 *
 ***************************************************************************/

int stMemCmp(const PVOID s1, const PVOID s2, SIZE n)
{
  register SIZE Len = n;
  register const UINT8 FAR *Ptr1 = (const UINT8 FAR *) s1;
  register const UINT8 FAR *Ptr2 = (const UINT8 FAR *) s2;

  /* Iterate through the block */
  while(Len > 0)
  {
    /* Difference found */
    if(*Ptr1 != *Ptr2)
      return (int) ((UINT8) *Ptr1 - (UINT8) *Ptr2);

    Ptr1++;
    Ptr2++;

    Len--;
  }

  /* Blocks are identical */
  return 0;
}


/****************************************************************************
 *
 *  Name:
 *    stMemCpy
 *
 *  Description:
 *    Copies n bytes from the source to the destination memory area.
 *    Handles overlapping memory regions correctly (like memmove).
 *
 *  Parameters:
 *    dest - Pointer to the destination memory block.
 *    src - Pointer to the source memory block.
 *    n - Number of bytes to copy.
 *
 *  Return:
 *    Pointer to the destination buffer.
 *
 ***************************************************************************/

PVOID stMemCpy(PVOID dest, const PVOID src, SIZE n)
{
  register SIZE Len = n;
  register UINT8 FAR *Ptr1 = (UINT8 FAR *) dest;
  register const UINT8 FAR *Ptr2 = (const UINT8 FAR *) src;

  /* Copy forward (Source is higher or non-overlapping) */
  if(Ptr1 < Ptr2)
    while(Len > 0)
    {
      *Ptr1 = *Ptr2;

      Ptr1++;
      Ptr2++;

      Len--;
    }

  /* Copy backward (Source is lower and overlaps) */
  else
  {
    Ptr1 += Len - 1;
    Ptr2 += Len - 1;

    while(Len > 0)
    {
      *Ptr1 = *Ptr2;

      Ptr1--;
      Ptr2--;

      Len--;
    }
  }

  return dest;
}


/****************************************************************************
 *
 *  Name:
 *    stMemSet
 *
 *  Description:
 *    Sets the first n bytes of the memory area pointed to by s to the
 *    specified value c.
 *
 *  Parameters:
 *    s - Pointer to the memory buffer.
 *    c - Value to set.
 *    n - Number of bytes.
 *
 *  Return:
 *    Pointer to the memory area s.
 *
 ***************************************************************************/

PVOID stMemSet(PVOID s, UINT8 c, SIZE n)
{
  register SIZE Len = n;
  register UINT8 Value = c;
  register UINT8 FAR *Ptr = (UINT8 FAR *) s;

  /* Fill the memory block */
  while(Len > 0)
  {
    *Ptr = Value;

    Ptr++;

    Len--;
  }

  return s;
}


/***************************************************************************/
#endif /* !ST_USE_MEMORY_CLIB */
/***************************************************************************/


/****************************************************************************
 *
 *  String Operations
 *
 ***************************************************************************/


/***************************************************************************/
#if !(ST_USE_STRING_CLIB)
/***************************************************************************/


/****************************************************************************
 *
 *  Name:
 *    stStrLen
 *
 *  Description:
 *    Computes the length of a string (excluding the null terminator).
 *
 *  Parameters:
 *    s - Pointer to the string.
 *
 *  Return:
 *    Number of characters in the string.
 *
 ***************************************************************************/

int stStrLen(const PSTR s)
{
  register int Len = 0;
  register PSTR Ptr = (PSTR) s;

  /* Scan for the null terminator */
  while(*Ptr)
  {
    Len++;
    Ptr++;
  }

  return Len;
}


/****************************************************************************
 *
 *  Name:
 *    stStrCpy
 *
 *  Description:
 *    Copies the source string to the destination.
 *
 *  Parameters:
 *    dest - Pointer to the destination buffer.
 *    src - Pointer to the source string.
 *
 *  Return:
 *    Pointer to the destination string.
 *
 ***************************************************************************/

PSTR stStrCpy(PSTR dest, const PSTR src)
{
  register PSTR Ptr1 = dest;
  register PSTR Ptr2 = (PSTR) src;

  /* Copy characters including the null terminator */
  while(1)
  {
    *Ptr1 = *Ptr2;

    if(!*Ptr2)
      break;

    Ptr1++;
    Ptr2++;
  }

  return dest;
}


/****************************************************************************
 *
 *  Name:
 *    stStrNCpy
 *
 *  Description:
 *    Copies up to maxlen characters from the source string to the
 *    destination.
 *
 *  Parameters:
 *    dest - Pointer to the destination buffer.
 *    src - Pointer to the source string.
 *    maxlen - Maximum number of characters to copy.
 *
 *  Return:
 *    Pointer to the destination string.
 *
 ***************************************************************************/

PSTR stStrNCpy(PSTR dest, const PSTR src, SIZE maxlen)
{
  register SIZE MaxLen = maxlen;
  register PSTR Ptr1 = dest;
  register PSTR Ptr2 = (PSTR) src;

  /* Copy characters up to the limit */
  while(MaxLen > 0)
  {
    *Ptr1 = *Ptr2;

    if(!*Ptr2)
      break;

    Ptr1++;
    Ptr2++;

    MaxLen--;
  }

  *Ptr1 = '\0';
  return dest;
}


/****************************************************************************
 *
 *  Name:
 *    stStrCat
 *
 *  Description:
 *    Concatenates the source string to the end of the destination string.
 *
 *  Parameters:
 *    dest - Pointer to the destination string.
 *    src - Pointer to the source string.
 *
 *  Return:
 *    Pointer to the destination string.
 *
 ***************************************************************************/

PSTR stStrCat(PSTR dest, const PSTR src)
{
  register PSTR Ptr1 = dest;
  register PSTR Ptr2 = (PSTR) src;

  /* Locate the null terminator of the destination */
  while(*Ptr1)
    Ptr1++;

  /* Append source characters including the null terminator */
  while(1)
  {
    *Ptr1 = *Ptr2;

    if(!*Ptr2)
      break;

    Ptr1++;
    Ptr2++;
  }

  return dest;
}


/****************************************************************************
 *
 *  Name:
 *    stStrNCat
 *
 *  Description:
 *    Concatenates up to maxlen characters from the source string to the
 *    destination string.
 *
 *  Parameters:
 *    dest - Pointer to the destination string.
 *    src - Pointer to the source string.
 *    maxlen - Maximum number of characters to append.
 *
 *  Return:
 *    Pointer to the destination string.
 *
 ***************************************************************************/

PSTR stStrNCat(PSTR dest, const PSTR src, SIZE maxlen)
{
  register SIZE MaxLen = maxlen;
  register PSTR Ptr1 = dest;
  register PSTR Ptr2 = (PSTR) src;

  /* Locate the null terminator of the destination */
  while(*Ptr1)
    Ptr1++;

  /* Append characters up to the limit */
  while(MaxLen > 0)
  {
    *Ptr1 = *Ptr2;

    if(!*Ptr2)
      break;

    Ptr1++;
    Ptr2++;

    MaxLen--;
  }

  *Ptr1 = '\0';
  return dest;
}


/****************************************************************************
 *
 *  Name:
 *    stStrCmp
 *
 *  Description:
 *    Compares two strings lexicographically.
 *
 *  Parameters:
 *    s1 - Pointer to the first string.
 *    s2 - Pointer to the second string.
 *
 *  Return:
 *   - < 0 if s1 is less than s2
 *   - 0 if s1 is equal to s2
 *   - > 0 if s1 is greater than s2
 *
 ***************************************************************************/

int stStrCmp(const PSTR s1, const PSTR s2)
{
  register PSTR Ptr1 = (PSTR) s1;
  register PSTR Ptr2 = (PSTR) s2;

  /* Compare all characters */
  while(1)
  {
    /* Difference found */
    if(*Ptr1 != *Ptr2)
      return (int) ((UINT8) *Ptr1 - (UINT8) *Ptr2);

    /* End of string reached (match) */
    if(!*Ptr1)
      return 0;

    Ptr1++;
    Ptr2++;
  }
}


/****************************************************************************
 *
 *  Name:
 *    stStrNCmp
 *
 *  Description:
 *    Compares up to maxlen characters of two strings.
 *
 *  Parameters:
 *    s1 - Pointer to the first string.
 *    s2 - Pointer to the second string.
 *    maxlen - Maximum number of characters to compare.
 *
 *  Return:
 *    - < 0 if s1 is less than s2
 *    - 0 if s1 is equal to s2
 *    - > 0 if s1 is greater than s2
 *
 ***************************************************************************/

int stStrNCmp(const PSTR s1, const PSTR s2, SIZE maxlen)
{
  register SIZE MaxLen = maxlen;
  register PSTR Ptr1 = (PSTR) s1;
  register PSTR Ptr2 = (PSTR) s2;

  /* Compare characters up to the limit */
  while(MaxLen)
  {
    /* Difference found */
    if(*Ptr1 != *Ptr2)
      return (int) ((UINT8) *Ptr1 - (UINT8) *Ptr2);

    /* End of string reached (match) */
    if(!*Ptr1)
      return 0;

    Ptr1++;
    Ptr2++;

    MaxLen--;
  }

  /* Strings are equal within the limit */
  return 0;
}


/****************************************************************************
 *
 *  Name:
 *    stStrICmp
 *
 *  Description:
 *    Compares two strings ignoring case.
 *
 *  Parameters:
 *    s1 - Pointer to the first string.
 *    s2 - Pointer to the second string.
 *
 *  Return:
 *    - < 0 if s1 is less than s2
 *    - 0 if s1 is equal to s2
 *    - > 0 if s1 is greater than s2
 *
 ***************************************************************************/

int stStrICmp(const PSTR s1, const PSTR s2)
{
  register PSTR Ptr1 = (PSTR) s1;
  register PSTR Ptr2 = (PSTR) s2;

  /* Compare all characters */
  while(1)
  {
    register char ch1 = *Ptr1;
    register char ch2 = *Ptr2;

    /* If characters differ */
    if(ch1 != ch2)
    {
      /* Normalize to uppercase for comparison */
      if(ch1 >= 'a' && ch1 <= 'z')
        ch1 -= (char)('a' - 'A');

      if(ch2 >= 'a' && ch2 <= 'z')
        ch2 -= (char)('a' - 'A');

      /* Check again after normalization */
      if(ch1 != ch2)
        return (int) ((UINT8) ch1 - (UINT8) ch2);
    }

    /* End of strings reached */
    if(!ch1)
      return 0;

    Ptr1++;
    Ptr2++;
  }
}


/****************************************************************************
 *
 *  Name:
 *    stStrNICmp
 *
 *  Description:
 *    Compares up to maxlen characters of two strings ignoring case.
 *
 *  Parameters:
 *    s1 - Pointer to the first string.
 *    s2 - Pointer to the second string.
 *    maxlen - Maximum number of characters to compare.
 *
 *  Return:
 *    - < 0 if s1 is less than s2
 *    - 0 if s1 is equal to s2
 *    - > 0 if s1 is greater than s2
 *
 ***************************************************************************/

int stStrNICmp(const PSTR s1, const PSTR s2, SIZE maxlen)
{
  register SIZE MaxLen = maxlen;
  register PSTR Ptr1 = (PSTR) s1;
  register PSTR Ptr2 = (PSTR) s2;

  /* Compare characters up to the limit */
  while(MaxLen)
  {
    register char ch1 = *Ptr1;
    register char ch2 = *Ptr2;

    /* If characters differ */
    if(ch1 != ch2)
    {
      /* Normalize to uppercase for comparison */
      if(ch1 >= 'a' && ch1 <= 'z')
        ch1 -= (char)('a' - 'A');

      if(ch2 >= 'a' && ch2 <= 'z')
        ch2 -= (char)('a' - 'A');

      /* Check again after normalization */
      if(ch1 != ch2)
        return (int) ((UINT8) ch1 - (UINT8) ch2);
    }

    /* End of strings reached */
    if(!ch1)
      return 0;

    Ptr1++;
    Ptr2++;

    MaxLen--;
  }

  /* Strings are equal within the limit */
  return 0;
}


/****************************************************************************
 *
 *  Name:
 *    stStrUpr
 *
 *  Description:
 *    Converts lowercase characters in a string to uppercase in place.
 *
 *  Parameters:
 *    s - Pointer to the string to convert.
 *
 *  Return:
 *    Pointer to the modified string.
 *
 ***************************************************************************/

PSTR stStrUpr(PSTR s)
{
  register PSTR Ptr = s;

  /* Iterate through the string */
  while(*Ptr)
  {
    /* Convert lowercase to uppercase */
    if(*Ptr >= 'a' && *Ptr <= 'z')
      *Ptr -= (char)('a' - 'A');

    Ptr++;
  }

  return s;
}


/***************************************************************************/
#endif /* !ST_USE_STRING_CLIB */
/***************************************************************************/
