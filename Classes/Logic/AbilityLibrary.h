#ifndef __UNITLIBRARY_H__
#define __UNITLIBRARY_H__

#include "MultiRefObject.h"


class CAbilityLibrary : public CMultiRefObject
{
public:
    CAbilityLibrary();
    M_SINGLETON(CAbilityLibrary);

    typedef CMultiRefMap<CAbility*> MAP_ABILITYS;
    M_SYNTHESIZE_READONLY_PASS_BY_REF(MAP_ABILITYS, m_mapTemplateAbilitys, TemplateAbilitys);
    int addTemplateAbility(CAbility* pAbility);
    int addTemplateAbility(int id, CAbility* pAbility);
    void loadTemplateAbilitys();
    virtual CAbility* copyAbility(int id) const;
};


#endif // __UNITLIBRARY_H__