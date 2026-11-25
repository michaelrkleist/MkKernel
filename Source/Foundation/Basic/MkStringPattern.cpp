////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Foundation\MkFoundationPCH.h"
#include "Foundation\Basic\MkStringPattern.h"

// Wx includes.
#include "Foundation\Math\MkOperators.h"

// Static initializations.
namespace
{
const std::shared_ptr<MkStringPattern::Molecule> srNullMolecule;
const std::shared_ptr<MkStringPattern::Alternates> srNullAlternates;
const std::shared_ptr<MkStringPattern::Marker> srNullMarker;

// NOTE: The leading ' ' is NOT a key character.  MRK.
const MkConstAString sKeyCharacters = MkConstAString(" ?[]-~*+{|}%$'@");

const MkAString sPatternEnd({MkStringPattern::ePatternEnd});
const MkAString sNOrMore({MkStringPattern::eAsterisk, MkStringPattern::ePlus});
const MkAString sAlternateEnd({MkStringPattern::eVerticalBar, MkStringPattern::eCloseBrace});
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkStringPattern::AnyCharacter
////////////////////////////////////////////////////////////////////////////////////////////////////

const std::shared_ptr<MkStringPattern::Molecule> MkStringPattern::AnyCharacter::Parse(
        const MkAString& metaPattern, mkuint32& charIdx)
{
    ++charIdx;  // Gobble up the question mark.

    // Check for zero or more key character which is the asterisk.
    if (metaPattern[charIdx] == MkStringPattern::eAsterisk)
    {
        ++charIdx;
        return std::make_shared<MkStringPattern::NOrMoreAny>(0, true);
    }
    else if (metaPattern[charIdx] == MkStringPattern::ePlus)
    {
        ++charIdx;
        return std::make_shared<MkStringPattern::NOrMoreAny>(0, false);
    }

    return std::make_shared<MkStringPattern::AnyCharacter>();
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkStringPattern::Character
////////////////////////////////////////////////////////////////////////////////////////////////////

const std::shared_ptr<MkStringPattern::Molecule> MkStringPattern::Character::Parse(
        const MkAString& metaPattern, mkuint32& charIdx)
{
    // Some complexity here.  The possible molecules are a character, no or more
    // characters, or a character string.
    mkuint32 beginCharIdx = charIdx;

    // NOTE: A very, very rare occasion for me, this line of code is.  MRK.
    while (metaPattern[++charIdx] > ePatternEnd) { }

    if (charIdx - beginCharIdx == 1)
    {
        if (sNOrMore.Find(metaPattern[charIdx]) >= 0)
        {
            ++charIdx;
            return std::make_shared<MkStringPattern::NOrMoreCharacter>(
                    metaPattern[beginCharIdx], 0, sNOrMore.Find(metaPattern[charIdx - 1]) == 0);  
        }

        return std::make_shared<MkStringPattern::Character>(metaPattern[beginCharIdx]);
    }
    else
    {
        if (sNOrMore.Find(metaPattern[charIdx]) >= 0)
        {
            // Back up a character so that it can be made N or more in a subsequent parse.
            --charIdx;
        }

        return std::make_shared<MkStringPattern::CharacterString>(
                MkAString(metaPattern, beginCharIdx, charIdx - beginCharIdx));
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkStringPattern::Characters
////////////////////////////////////////////////////////////////////////////////////////////////////

const std::shared_ptr<MkStringPattern::Molecule> MkStringPattern::Characters::Parse(
        const MkAString& metaPattern, mkuint32& charIdx)
{
    // Some complexity here.  The possible molecules are a list of characters, one or
    // more character ranges, or no or more of either.
    
    // Check for absence.
    bool present = true;
    if (metaPattern[++charIdx] == eTilde)
    {
        present = false;
        ++charIdx;
    }

    // Count the number of pattern characters before the closing bracket.
    mkuint32 patternCharIdx = charIdx;
    while (metaPattern[patternCharIdx] != eCloseBracket)
    {
        if (patternCharIdx >= metaPattern.GetCharacterCount() ||  
                (metaPattern[patternCharIdx] <= ePatternEnd && metaPattern[patternCharIdx] != eHyphen))
        {
            return srNullMolecule;
        }
        ++patternCharIdx;
    }
    mkuint32 patternCharCnt = patternCharIdx - charIdx;
    if (patternCharCnt == 0) return srNullMolecule;

    // Check if there are ranges.
    if (patternCharCnt < 2 || metaPattern[charIdx + 1] != eHyphen)
    {
        mkuint32 beginCharIdx = charIdx;
        --charIdx;          // Back up to make iteration work cleaner.

        // We should just have a list of characters.
        // NOTE: A very, very rare occasion for me, this line of code is.  MRK.
        while (metaPattern[++charIdx] > ePatternEnd) { }

        // Check if we found the closing bracket.
        if (metaPattern[charIdx] == eCloseBracket)
        {
            auto rAtom = std::make_shared<Atom>(present);

            MkAString characters(metaPattern, beginCharIdx, charIdx - beginCharIdx);
            rAtom->AddProton(std::make_shared<CharacterList>(CharacterList(characters)));

            // Check for no or more.
            if (sNOrMore.Find(metaPattern[++charIdx]) >= 0)
            {
                ++charIdx;

                return std::make_shared<MkStringPattern::NOrMoreCharacters>(
                        rAtom, 0, sNOrMore.Find(metaPattern[charIdx - 1]) == 0);
            }

            return std::make_shared<MkStringPattern::Characters>(rAtom);
        }

        return srNullMolecule;
    }
    else
    {
        // There needs to be triplets.
        if (patternCharCnt % 3 != 0) return srNullMolecule;

        auto rAtom = std::make_shared<Atom>(present);

        // Process the triplets.
        while (patternCharCnt > 0)
        {
            if (metaPattern[charIdx + 1] != eHyphen) return srNullMolecule;

            mkchar8 char0 = metaPattern[charIdx];
            mkchar8 char1 = metaPattern[charIdx + 2];
            if (char0 <= ePatternEnd || char1 <= ePatternEnd) return srNullMolecule;

            rAtom->AddProton(std::make_shared<CharacterRange>(MkMin(char0, char1), MkMax(char0, char1)));

            charIdx += 3;
            patternCharCnt -= 3;
        }

        // Check for no or more.
        if (sNOrMore.Find(metaPattern[++charIdx]) >= 0)
        {
            ++charIdx;

            return std::make_shared<MkStringPattern::NOrMoreCharacters>(
                    rAtom, 0, sNOrMore.Find(metaPattern[charIdx - 1]) == 0);
        }

        return std::make_shared<MkStringPattern::Characters>(rAtom);
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkStringPattern::NOrMoreAny
////////////////////////////////////////////////////////////////////////////////////////////////////

MkStringPattern::MatchResult MkStringPattern::NOrMoreAny::Match(
        const MkAString& string, mkuint32 charIdx)
{
    m_beginCharIdx = charIdx;

    if (Greedy())
    {
        m_endCharIdx = string.GetCharacterCount();
        return MatchResult(bool(m_endCharIdx - m_beginCharIdx >= m_minCharCnt), m_endCharIdx);
    }
    else
    {
        m_endCharIdx = MkMin(m_beginCharIdx + m_minCharCnt, string.GetCharacterCount());
        return MatchResult(bool(m_endCharIdx - m_beginCharIdx == m_minCharCnt), m_endCharIdx);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

MkStringPattern::MatchResult MkStringPattern::NOrMoreAny::NextMatch(const MkAString& string)
{
    if (Greedy())
    {
        if (m_endCharIdx == m_beginCharIdx + m_minCharCnt) return MatchResult(false, m_endCharIdx);
        return MatchResult(true, --m_endCharIdx);
    }
    else
    {
        if (++m_endCharIdx > string.GetCharacterCount()) return MatchResult(false, string.GetCharacterCount());
        return MatchResult(true, m_endCharIdx);
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkStringPattern::NOrMoreCharacter
////////////////////////////////////////////////////////////////////////////////////////////////////

MkStringPattern::MatchResult MkStringPattern::NOrMoreCharacter::Match(
        const MkAString& string, mkuint32 charIdx)
{
    m_beginCharIdx = charIdx;
    m_endCharIdx = charIdx;

    if (Greedy())
    {
        while (m_endCharIdx < string.GetCharacterCount())
        {
            if (string[m_endCharIdx] != m_character)
            {
                return MatchResult(bool(m_endCharIdx - m_beginCharIdx >= m_minCharCnt), m_endCharIdx);
            }
            ++m_endCharIdx;
        }

        return MatchResult(bool(m_endCharIdx - m_beginCharIdx >= m_minCharCnt), m_endCharIdx);
    }
    else
    {
        while (m_endCharIdx - m_beginCharIdx < m_minCharCnt)
        {
            if (string[m_endCharIdx] != m_character)
            {
                return MatchResult(false, m_endCharIdx);
            }

            if (++m_endCharIdx >= string.GetCharacterCount())
            {
                return MatchResult(false, string.GetCharacterCount());
            }
        }

        return MatchResult(true, m_endCharIdx);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

MkStringPattern::MatchResult MkStringPattern::NOrMoreCharacter::NextMatch(const MkAString& string)
{
    if (Greedy())
    {
        if (m_endCharIdx == m_beginCharIdx + m_minCharCnt) return MatchResult(false, m_endCharIdx);
        return MatchResult(true, --m_endCharIdx);
    }
    else
    {
        if (m_endCharIdx >= string.GetCharacterCount() || string[m_endCharIdx] != m_character)
        {
            return MatchResult(false, m_endCharIdx);
        }
        return MatchResult(true, ++m_endCharIdx);
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkStringPattern::NOrMoreCharacters
////////////////////////////////////////////////////////////////////////////////////////////////////

MkStringPattern::MatchResult MkStringPattern::NOrMoreCharacters::Match(
        const MkAString& string, mkuint32 charIdx)
{
    m_beginCharIdx = charIdx;
    m_endCharIdx = charIdx;

    if (Greedy())
    {
        while (m_endCharIdx < string.GetCharacterCount())
        {
            if (!m_rAtom->Match(string[m_endCharIdx]))
            {
                return MatchResult(bool(m_endCharIdx - m_beginCharIdx >= m_minCharCnt), m_endCharIdx);
            }
            ++m_endCharIdx;
        }

        return MatchResult(bool(m_endCharIdx - m_beginCharIdx >= m_minCharCnt), m_endCharIdx);
    }
    else
    {
        while (m_endCharIdx - m_beginCharIdx < m_minCharCnt)
        {
            if (!m_rAtom->Match(string[m_endCharIdx])) return MatchResult(false, m_endCharIdx);

            if (++m_endCharIdx >= string.GetCharacterCount())
            {    
                return MatchResult(false, string.GetCharacterCount());
            }
        }

        return MatchResult(true, m_endCharIdx);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

MkStringPattern::MatchResult MkStringPattern::NOrMoreCharacters::NextMatch(const MkAString& string)
{
    if (Greedy())
    {
        if (m_endCharIdx == m_beginCharIdx + m_minCharCnt) return MatchResult(false, m_endCharIdx);
        return MatchResult(true, --m_endCharIdx);
    }
    else
    {
        if (m_endCharIdx >= string.GetCharacterCount() || !m_rAtom->Match(string[m_endCharIdx]))
        {
            return MatchResult(false, m_endCharIdx);
        }
        return MatchResult(true, ++m_endCharIdx);
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkStringPattern::Sequence
////////////////////////////////////////////////////////////////////////////////////////////////////

MkStringPattern::MatchResult MkStringPattern::Sequence::Match(const MkAString& string, mkuint32 charIdx)
{
    mkuint32 beginCharIdx = charIdx;
    m_moleculeIdx = 0;

    while (true)
    {
        MatchResult advanceMatch = AdvanceMatch(string, charIdx);
        if (advanceMatch.m_matched) return advanceMatch;

        MatchResult retreatMatch = RetreatMatch(string);

        // Tried every variation, give up.
        if (!retreatMatch.m_matched) return MatchResult(false, beginCharIdx);

        // NOTE: This line is just ugly hanging out here.  MRK.
        charIdx = retreatMatch.m_nextCharIdx;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

MkStringPattern::MatchResult MkStringPattern::Sequence::NextMatch(const MkAString& string)
{
    while (true)
    {
        // Look for the next match in the current molecule.
        MatchResult nextMatch = m_molecules[m_moleculeIdx]->NextMatch(string);

        if (nextMatch.m_matched) return nextMatch;

        // If there wasn't a next match on the first molecule, we are done.
        if (m_moleculeIdx == 0) break;

        // No more matches in this molecule, try the previous one.
        --m_moleculeIdx;
    }

    return MatchResult(false, 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

const std::shared_ptr<MkStringPattern::Marker> MkStringPattern::Sequence::GetMarker(
        const mkuint32 markerIdx) const
{
    mkuint32 markerCnt = 0;
    for (mkuint32 moleculeIdx = 0; moleculeIdx < (mkuint32)m_molecules.size(); ++moleculeIdx)
    {
        if (m_molecules[moleculeIdx]->GetType() == eMarker)
        {
            if (markerIdx == markerCnt)
            {
                return std::static_pointer_cast<Marker>(m_molecules[moleculeIdx]);
            }

            ++markerCnt;
        }
    }

    return std::shared_ptr<MkStringPattern::Marker>();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

const std::shared_ptr<MkStringPattern::Alternates> MkStringPattern::Sequence::GetAlternates(
        const mkuint32 alternatesIdx) const
{
    mkuint32 alternatesCnt = 0;
    for (mkuint32 moleculeIdx = 0; moleculeIdx < (mkuint32)m_molecules.size(); ++moleculeIdx)
    {
        if (m_molecules[moleculeIdx]->GetType() == eAlternates)
        {
            if (alternatesIdx == alternatesCnt)
            {
                return std::static_pointer_cast<Alternates>(m_molecules[moleculeIdx]);
            }

            ++alternatesCnt;
        }
    }

    return std::shared_ptr<MkStringPattern::Alternates>();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

MkStringPattern::MatchResult MkStringPattern::Sequence::AdvanceMatch(
        const MkAString& string, mkuint32& charIdx)
{
    // Advance through molecules.
    while (m_moleculeIdx < (mkuint32)m_molecules.size())
    {
        MatchResult advanceMatch = m_molecules[m_moleculeIdx]->Match(string, charIdx);
        if (advanceMatch.m_matched)
        {
            // The easy case, everything is going smoothly.
            charIdx = advanceMatch.m_nextCharIdx;
            ++m_moleculeIdx;
        }
        else
        {
            // Could not match a molecule, return the failure.
            return MatchResult(false, charIdx);
        }
    }
    
    return MatchResult(true, charIdx);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

MkStringPattern::MatchResult MkStringPattern::Sequence::RetreatMatch(const MkAString& string)
{
    // A molecule match in the sequence failed, we need to back up to N or mores,
    // sequences and alternates and try the next option.  If we have any.
    // NOTE: The molecule index is either
    //   The molecule match that failed if we are evaluating the main (non-alternate)
    //     sequence.
    //   One beyond the last molecule in an alternate sequence that we for which we
    //     are trying a different match.
    //  MRK.
    // NOTE: While retreating we only want to test molecules that can do variable
    // matches, that is, N or mores, sequences and alternates.
    while (m_moleculeIdx > 0)
    {
        --m_moleculeIdx;

        if (m_molecules[m_moleculeIdx]->Variable())
        {
            MatchResult retreatMatch = m_molecules[m_moleculeIdx]->NextMatch(string);
            if (retreatMatch.m_matched)
            {
                ++m_moleculeIdx;
                return retreatMatch;
            }
        }
    }

    return MatchResult(false, 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool MkStringPattern::Sequence::Parse(
        const MkAString& metaPattern, mkuint32& charIdx, const MkAString& endChars)
{
    while (!PatternEnd(metaPattern, charIdx, endChars))
    {
        switch (metaPattern[charIdx])
        {
        case eQuestionMark :
            AddMolecule(AnyCharacter::Parse(metaPattern, charIdx));
            break;

        case eOpenBracket :
        {
            auto rMolecule = MkStringPattern::Characters::Parse(metaPattern, charIdx);

            // Invalid character set/character range.
            if (rMolecule == nullptr) return false;

            AddMolecule(rMolecule);
            break;
        }

        case eOpenBrace :
        {
            auto rMolecule = MkStringPattern::Alternates::Parse(metaPattern, charIdx, endChars);

            // Invalid alternate set.
            if (rMolecule == nullptr) return false;

            AddMolecule(rMolecule);
            break;
        }

        // NOTE: Incomplete error checking here.  MRK.
        case ePerCent :
            ++charIdx;
            AddMolecule(std::make_shared<MkStringPattern::StringBegin>());
            break;

        case eDollarSign :
            ++charIdx;
            AddMolecule(std::make_shared<MkStringPattern::StringEnd>());
            break;

        case eApostrophe :
            ++charIdx;
            AddMolecule(std::make_shared<MkStringPattern::Marker>());
            break;

        default:
            // Unexpected key character.
            if (metaPattern[charIdx] <= (mkchar8)ePatternEnd) return false;

             AddMolecule(Character::Parse(metaPattern, charIdx));
            break;
        }
    };

    return true;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkStringPattern::Alternates
////////////////////////////////////////////////////////////////////////////////////////////////////

MkStringPattern::MatchResult MkStringPattern::Alternates::Match(const MkAString& string, mkuint32 charIdx)
{
    m_beginCharIdx = charIdx;

    for (m_alternateIdx = 0; m_alternateIdx < (mkuint32)m_alternates.size(); ++m_alternateIdx)
    {
        charIdx = m_beginCharIdx;
        MatchResult alternateMatch = m_alternates[m_alternateIdx].Match(string, charIdx);
        if (alternateMatch.m_matched) return alternateMatch;
    }

    return MatchResult(false, m_beginCharIdx);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

MkStringPattern::MatchResult MkStringPattern::Alternates::NextMatch(const MkAString& string)
{
    // Keep trying the current alternate until variations are exhausted.
    while (true)
    {
        MatchResult retreatMatch = m_alternates[m_alternateIdx].RetreatMatch(string);
        if (!retreatMatch.m_matched) break;

        mkuint32 charIdx = retreatMatch.m_nextCharIdx;
        MatchResult advanceMatch = m_alternates[m_alternateIdx].AdvanceMatch(string, charIdx);
        if (advanceMatch.m_matched) return advanceMatch;
    }

    // Tried every variation of the current alternate, try the next alternate.
    mkuint32 charIdx = m_beginCharIdx;
    if (++m_alternateIdx >= (mkuint32)m_alternates.size()) return MatchResult(false, m_beginCharIdx);

    MatchResult alternateMatch = m_alternates[m_alternateIdx].Match(string, charIdx);
    if (alternateMatch.m_matched) return alternateMatch;

    return MatchResult(false, 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

const std::shared_ptr<MkStringPattern::Molecule> MkStringPattern::Alternates::Parse(
        const MkAString& metaPattern, mkuint32& charIdx, const MkAString& endChars)
{
    auto rAlternates = std::make_shared<MkStringPattern::Alternates>();

    ++charIdx;
    mkuint32 alternateIdx = 0;
    while (true)
    {
        rAlternates->AddAlternate(MkStringPattern::Sequence());
        if (!rAlternates->GetAlternate(alternateIdx).Parse(metaPattern, charIdx, sAlternateEnd))
        {
            return srNullMolecule;
        }

        if (metaPattern[charIdx] == (mkchar8)eCloseBrace)
        {
            // Alternates with one choice make no sense.
            if (rAlternates->GetCount() < 2) return srNullMolecule;

            ++charIdx;
            return rAlternates;
        }

        ++charIdx;
        ++alternateIdx;
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkStringPattern
////////////////////////////////////////////////////////////////////////////////////////////////////

MkStringPattern::MkStringPattern(const MkAString& pattern)
{
    MkAString metaPattern = TranslatePattern(pattern);

    mkuint32 charIdx = 0;
    if (!m_moleculeSequence.Parse(metaPattern, charIdx, sPatternEnd))
    {
        m_moleculeSequence = Sequence();
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////

MkAString MkStringPattern::TranslatePattern(const MkAString& pattern) const
{
    MkAString metaPattern;

    for (mkuint32 charIdx = 0; charIdx < pattern.GetCharacterCount(); ++charIdx)
    {
        mkint keyIdx = sKeyCharacters.Find(pattern[charIdx]);

        if (keyIdx < 0)
        {
            metaPattern += pattern[charIdx];
        }
        else if (keyIdx == (mkint)eAtSign)
        {
            if (++charIdx < pattern.GetCharacterCount())
            {
                metaPattern += pattern[charIdx];
            }
            else
            {
                return MkAString();
            }
        }
        else
        {
            metaPattern += (mkchar8)keyIdx;
        }
    }

    metaPattern += ePatternEnd;

    return metaPattern;
}


////////////////////////////////////////////////////////////////////////////////////////////////////

MkStringPattern::Sequence::Sequence() :
    Molecule(eSequence),
    m_moleculeIdx(0)
{
    static mkuint sTag;
    m_tag = sTag++;
}