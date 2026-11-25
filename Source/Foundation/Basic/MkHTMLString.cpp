////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2026 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////
 
#include "Foundation\MkFoundationPCH.h"
#include "Basic\MkHTMLString.h"

// Mk includes.

// Static initializations.
namespace
{
mkchar8 sSlashChar = '/';
mkchar8 sLessThanChar = '<';
mkchar8 sMoreThanChar = '>';
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkHTMLString
////////////////////////////////////////////////////////////////////////////////////////////////////

mkuint32 MkHTMLString::Parse(const MkAString& tagStr, const MkAString& string, const mkuint32 startIdx)
{
    mkuint32 charIdx = startIdx;
    mkint tagCnt = 0;
    mkuint32 tagStartIdx = 0;
    while (startIdx < string.GetCharacterCount())
    {
        if (string[startIdx] == sLessThanChar)
        {
            ++charIdx;

            bool openTag = true;
            if (string[startIdx] == sSlashChar)
            {
                openTag = false;
                ++charIdx;
            }

            if (string.Match(tagStr, startIdx))
            {
                if (openTag)
                {
                    if (tagCnt == 0)
                    {
                        tagStartIdx = startIdx - 1;
                    }
                    ++tagCnt;
                }
                else
                {
                    if (tagCnt == 0)
                    {
                        m_htmlString = MkAString();
                        return 0;
                    }

                    if (--tagCnt == 0)
                    {
                        // Move index beyond the '>' character.
                        charIdx += tagStr.GetCharacterCount() + 1;
                        mkuint32 charCnt = charIdx - tagStartIdx;
                        m_htmlString = MkAString(string, charIdx, charCnt);
                        return charIdx;
                    }
                }
            }
        }

        ++charIdx;
    }

    m_htmlString = MkAString();
    return 0;
}
