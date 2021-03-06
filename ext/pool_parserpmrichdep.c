/*
 * Copyright (c) 2015, SUSE Inc.
 *
 * This program is licensed under the BSD license, read LICENSE.BSD
 * for further information
 */

/* this is used by repo_rpmmd, repo_rpmdb, and repo_susetags */

#include <stdio.h>

#include "pool.h"
#include "pool_parserpmrichdep.h"

static struct RichOpComp {
  const char *n;
  int l;
  Id fl;
} RichOps[] = {
  { "and",     3, REL_AND },
  { "or",      2, REL_OR },
  { "if",      2, REL_COND },
  { "unless",  6, REL_UNLESS },
  { "else",    4, REL_ELSE },
  { "with",    4, REL_WITH },
  { "without", 7, REL_WITHOUT },
  { NULL, 0, 0},
};

static inline const char *
skipnonwhite(const char *p)
{
  int bl = 0;
  while (*p && !(*p == ' ' || *p == ',' || (*p == ')' && bl-- <= 0)))
    if (*p++ == '(')
      bl++;
  return p;
}

static Id
parseRichDep(Pool *pool, const char **depp, Id chainfl)
{
  const char *p = *depp;
  const char *n;
  Id id, evr;
  int fl;
  struct RichOpComp *op;

  if (!chainfl && *p++ != '(')
    return 0;
  while (*p == ' ')
    p++;
  if (*p == ')')
    return 0;
  if (*p == '(')
    {
      id = parseRichDep(pool, &p, 0);
      if (!id)
	return 0;
    }
  else
    {
      n = p;
      p = skipnonwhite(p);
      if (n == p)
	return 0;
      id = pool_strn2id(pool, n, p - n, 1);
      while (*p == ' ')
	p++;
      if (*p)
	{
	  fl = 0;
	  for (;; p++)
	    {
	      if (*p == '<')
		fl |= REL_LT;
	      else if (*p == '=')
		fl |= REL_EQ;
	      else if (*p == '>')
		fl |= REL_GT;
	      else
		break;
	    }
	  if (fl)
	    {
	      while (*p == ' ')
		p++;
	      n = p;
	      p = skipnonwhite(p);
	      if (p - n > 2 && n[0] == '0' && n[1] == ':')
		n += 2;		/* strip zero epoch */
	      if (n == p)
		return 0;
	      id = pool_rel2id(pool, id, pool_strn2id(pool, n, p - n, 1), fl, 1);
	    }
	}
    }
  while (*p == ' ')
    p++;
  if (!*p)
    return 0;
  if (*p == ')')
    {
      *depp = p + 1;
      return id;
    }
  n = p;
  while (*p && *p != ' ')
    p++;
  for (op = RichOps; op->n; op++)
    if (p - n == op->l && !strncmp(n, op->n, op->l))
      break;
  fl = op->fl;
  if (!fl)
    return 0;
  if ((chainfl == REL_COND || chainfl == REL_UNLESS) && fl == REL_ELSE)
    chainfl = 0;
  if (chainfl && fl != chainfl)
    return 0;
  evr = parseRichDep(pool, &p, fl);
  if (!evr)
    return 0;
  *depp = p;
  return pool_rel2id(pool, id, evr, fl, 1);
}

Id
pool_parserpmrichdep(Pool *pool, const char *dep)
{
  Id id = parseRichDep(pool, &dep, 0);
  if (id && *dep)
    id = 0;
  return id;
}

