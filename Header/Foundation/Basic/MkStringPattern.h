///////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2025 by Michael R. Kleist
///////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

// Mk includes.
#include "MkFoundation.h"
#include "Basic/MkString.h"

// Pattern prototype syntax.
//      ?           Match any character.  Optional */+.
//      x           Match the string "x".  Optional */+.
//      xxx         Match the string "xxx".
//      [xxx]       Match any of the characters in "xxx".  Optional */+.
//      [~xxx]      Match any character except those in "xxx".  Optional */+.
//      [a-b]       Match any character from "a" to "b".  Optional */+.
//      [~a-b]      (Take a wild guess.)  Optional */+.
//      [a-bc-d...]     Match any characters in any range.  Optional */+.
//      [~a-bc-d...]    (Take another wild guess.)  Optional */+.
//      * suffix        Match zero or more of the preceding pattern greedily.  E.g. ?*, [0-9]*
//      + suffix        Match zero or more of the preceding pattern cheaply.  E.g. ?+, [0-9]+
//      {p0|p1|p2...}   Match one of the patterns "pn" starting with the first one.
//      %           Match the beginning of the string.
//      $           Match the end of the string.
//      '           Mark this character position in the string.
//      @           Escape character for using key characters as literal characters.
//
//      ? [ ] - ~ * + { | } % $ ' @     Complete key character set.
//
// NOTE: The code supports N or more, not just zero or more, but I can't think of
// a syntax that is easy to parse to support this capability.  MRK.
// NOTE: "Greedily means match as many as possible, "cheaply" means match as few as
// possible.  MRK.


////////////////////////////////////////////////////////////////////////////////////////////////////
// MkStringPattern
////////////////////////////////////////////////////////////////////////////////////////////////////

class MkStringPattern
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Proton
    ////////////////////////////////////////////////////////////////////////////////////////////////
    
    // Proton is a matching pattern for one subset of characters.
    class Proton
    {
    public:
        Proton() = default;
        ~Proton() = default;

        virtual bool Match(const mkchar8 character) const = 0;
    };


    ////////////////////////////////////////////////////////////////////////////////////////////////
    // CharacterList
    ////////////////////////////////////////////////////////////////////////////////////////////////

    class CharacterList : public Proton
    {
    public:
        CharacterList(const MkAString& characters) :
            m_characters(characters)
        { }

        bool Match(const mkchar8 character) const override
        { return bool(m_characters.Find(character) >= 0); }

    private:
        MkAString m_characters;
    };


    ////////////////////////////////////////////////////////////////////////////////////////////////
    // CharacterRange
    ////////////////////////////////////////////////////////////////////////////////////////////////

    class CharacterRange : public Proton
    {
    public:
        CharacterRange(const mkchar8 firstCharacter, const mkchar8 lastCharacter) :
            m_firstCharacter(firstCharacter),
            m_lastCharacter(lastCharacter)
        { }

        bool Match(const mkchar8 character) const override
        { return bool(character >= m_firstCharacter && character <= m_lastCharacter); }

    private:
        mkchar8 m_firstCharacter;
        mkchar8 m_lastCharacter;
    };


    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Atom
    ////////////////////////////////////////////////////////////////////////////////////////////////
    
    // An atom holds one ore more Protons.
    class Atom
    {
    public:
        Atom(const bool present) :
            m_present(present)
        { }
        ~Atom() = default;

        void AddProton(const std::shared_ptr<Proton>& rProton)
        { m_protons.push_back(rProton); }

        bool Match(const mkchar8 character) const
        {
            for (auto& protonIter : m_protons)
            {
                if (protonIter->Match(character)) return m_present;
            }
            return !m_present;
        }

    private:
        std::vector<std::shared_ptr<Proton>> m_protons;
        bool m_present;
    };


    ////////////////////////////////////////////////////////////////////////////////////////////////
    // MatchResult
    ////////////////////////////////////////////////////////////////////////////////////////////////

    struct MK_EXPORT_FOUNDATION MatchResult
    {
        MatchResult(const bool matched, const mkuint32 nextCharIdx) :
            m_matched(matched),
            m_nextCharIdx(nextCharIdx)
        { }

        bool m_matched;
        mkuint32 m_nextCharIdx;
    };


    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Molecule
    ////////////////////////////////////////////////////////////////////////////////////////////////
    
    // Molecule is a matching pattern for an arbitrary sequence of characters in the
    // character string, with some liberties for markers.  After a match, a molecule
    // maintains matching state for later query.
    class Molecule
    {
    public:
        // NOTE: This approach is crappy.  Make it object oriented.  MRK.
        enum Type
        {
            eMarker = 0,
            eNInstancesOrMoreGreedy,
            eNInstancesOrMoreCheap,
            eSequence,
            eAlternates,
            eOther
        };

        Molecule(const Type type = eOther) :
            m_type(type)
        { }

        ~Molecule() = default;

        // The molecule matches as much as it can, or in the case, of alternates, matches
        // just one of the alternates.
        virtual MatchResult Match(const MkAString& string, mkuint32 charIdx) = 0;

        // NextMatch asks a molecule to match less or more, or try another alternate.  It is
        // called when a subsequent molecule fails to match.
        virtual MatchResult NextMatch(const MkAString& string)
        { return MatchResult(false, 0); }

        Type GetType() const
        { return m_type; }

        bool Greedy() const
        { return bool(m_type == eNInstancesOrMoreGreedy); }

        bool Variable() const
        {
            return bool((m_type == eNInstancesOrMoreGreedy) ||
                    (m_type == eNInstancesOrMoreCheap) ||
                    (m_type == eSequence) ||
                    (m_type == eAlternates));
        }

    protected:
        Type m_type;
    };


    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Marker
    ////////////////////////////////////////////////////////////////////////////////////////////////
    
    class Marker : public Molecule
    {
    public:
        Marker() :
            Molecule(Type::eMarker)
        { }

        MatchResult Match(const MkAString& string, mkuint32 charIdx) override
        { m_charIdx = charIdx;  return MatchResult(true, charIdx); }

        mkuint32 GetCharacterIndex() const
        { return m_charIdx; }

    private:
        mkuint32 m_charIdx = 0;
    };


    ////////////////////////////////////////////////////////////////////////////////////////////////
    // StringBegin
    ////////////////////////////////////////////////////////////////////////////////////////////////
    
    class StringBegin : public Molecule
    {
    public:
        StringBegin() = default;

        MatchResult Match(const MkAString& string, mkuint32 charIdx) override
        { return MatchResult(bool(charIdx == 0), charIdx); }
    };


    ////////////////////////////////////////////////////////////////////////////////////////////////
    // StringEnd
    ////////////////////////////////////////////////////////////////////////////////////////////////
    
    class StringEnd : public Molecule
    {
    public:
        StringEnd() = default;

        MatchResult Match(const MkAString& string, mkuint32 charIdx) override
        { return MatchResult(bool(charIdx >= string.GetCharacterCount()), charIdx); }
    };


    ////////////////////////////////////////////////////////////////////////////////////////////////
    // AnyCharacter
    ////////////////////////////////////////////////////////////////////////////////////////////////
    
    class AnyCharacter : public Molecule
    {
    public:
        AnyCharacter() = default;

        MatchResult Match(const MkAString& string, mkuint32 charIdx) override
        { return MatchResult(true, charIdx + 1); }

        static const std::shared_ptr<Molecule> Parse(const MkAString& metaPattern, mkuint32& charIdx);
    };


    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Character
    ////////////////////////////////////////////////////////////////////////////////////////////////
    
    class Character : public Molecule
    {
    public:
        Character(const mkchar8& character) :
            m_character(character)
        { }

        MatchResult Match(const MkAString& string, mkuint32 charIdx) override
        {
            if (m_character == string[charIdx])
            {
                return MatchResult(true, ++charIdx);
            }
            else
            {
                return MatchResult(false, charIdx);
            }
        }

        static const std::shared_ptr<Molecule> Parse(const MkAString& metaPattern, mkuint32& charIdx);
            
    private:
        mkchar8 m_character;
    };


    ////////////////////////////////////////////////////////////////////////////////////////////////
    // CharacterString
    ////////////////////////////////////////////////////////////////////////////////////////////////
    
    class CharacterString : public Molecule
    {
    public:
        CharacterString(const MkAString& string) :
            m_string(string)
        { }

        MatchResult Match(const MkAString& string, mkuint32 charIdx) override
        {
            if (m_string.GetCharacterCount() > string.GetCharacterCount() - charIdx)
            {
                return MatchResult(false, charIdx);
            }

            mkuint32 beginCharIdx = charIdx;
            for (mkuint32 strIdx = 0; strIdx < m_string.GetCharacterCount(); ++strIdx, ++charIdx)
            {
                if (m_string[strIdx] != string[charIdx]) return MatchResult(false, beginCharIdx);
            }
            return MatchResult(true, charIdx);
        }
            
    private:
        MkAString m_string;
    };


    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Characters
    ////////////////////////////////////////////////////////////////////////////////////////////////
    
    class Characters : public Molecule
    {
    public:
        Characters(const std::shared_ptr<Atom>& rAtom) :
            m_rAtom(rAtom)
        { }

        MatchResult Match(const MkAString& string, mkuint32 charIdx) override
        { 
            if (m_rAtom->Match(string[charIdx])) return MatchResult(true, ++charIdx);
            return MatchResult(false, charIdx);
        }

        static const std::shared_ptr<Molecule> Parse(const MkAString& metaPattern, mkuint32& charIdx);
            
    private:
        std::shared_ptr<Atom> m_rAtom;
    };


    ////////////////////////////////////////////////////////////////////////////////////////////////
    // NOrMoreAny
    ////////////////////////////////////////////////////////////////////////////////////////////////
    
    class NOrMoreAny : public Molecule
    {
    public:
       NOrMoreAny(const mkuint32 minCharCnt, const bool greedy) :
            Molecule(greedy ? eNInstancesOrMoreGreedy : eNInstancesOrMoreCheap),
            m_minCharCnt(minCharCnt)
        { }

        MatchResult Match(const MkAString& string, mkuint32 charIdx) override;

        MatchResult NextMatch(const MkAString& string) override;
            
    private:
        mkuint32 m_minCharCnt;

        mkuint32 m_beginCharIdx = 0;
        mkuint32 m_endCharIdx = 0;
    };


    ////////////////////////////////////////////////////////////////////////////////////////////////
    // NOrMoreCharacter
    ////////////////////////////////////////////////////////////////////////////////////////////////
    
    class NOrMoreCharacter : public Molecule
    {
    public:
        NOrMoreCharacter(
                const mkchar8 character, const mkuint32 minCharCnt, const bool greedy) :
            Molecule(greedy ? eNInstancesOrMoreGreedy : eNInstancesOrMoreCheap),
            m_character(character),
            m_minCharCnt(minCharCnt)
        { }

        MatchResult Match(const MkAString& string, mkuint32 charIdx) override;

        MatchResult NextMatch(const MkAString& string) override;
            
    private:
        mkchar8 m_character;
        mkuint32 m_minCharCnt;

        mkuint32 m_beginCharIdx = 0;
        mkuint32 m_endCharIdx = 0;
    };


    ////////////////////////////////////////////////////////////////////////////////////////////////
    // NOrMoreCharacters
    ////////////////////////////////////////////////////////////////////////////////////////////////
    
    class NOrMoreCharacters : public Molecule
    {
    public:
        NOrMoreCharacters(const std::shared_ptr<Atom>& rAtom, const mkuint32 minCharCnt, const bool greedy) :
            Molecule(greedy ? eNInstancesOrMoreGreedy : eNInstancesOrMoreCheap),
            m_rAtom(rAtom),
            m_minCharCnt(minCharCnt)
        { }

        MatchResult Match(const MkAString& string, mkuint32 charIdx) override;

        MatchResult NextMatch(const MkAString& string) override;
            
    private:
        std::shared_ptr<Atom> m_rAtom;
        mkuint32 m_minCharCnt;

        mkuint32 m_beginCharIdx = 0;
        mkuint32 m_endCharIdx = 0;
    };


    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Sequence
    ////////////////////////////////////////////////////////////////////////////////////////////////
    
    // Forward declarations.
    class Alternates;

    // Molecules evaluated in sequence.
    class Sequence : public Molecule
    {
    public:
        Sequence();
#if 0
        Sequence() :
            Molecule(eSequence),
            m_moleculeIdx(0)
        { }
#endif

        MatchResult Match(const MkAString& string, mkuint32 charIdx) override;

        MatchResult NextMatch(const MkAString& string) override;

        // Fetch relevant molecules for match information.  Don't enter alternates.
        // NOTE: No protection against invalid requests.  MRK.
        const std::shared_ptr<Marker> MK_EXPORT_FOUNDATION GetMarker(const mkuint32 markerIdx) const;

        const std::shared_ptr<Alternates> MK_EXPORT_FOUNDATION GetAlternates(const mkuint32 alternatesIdx) const;
        const Sequence&  GetMatchedAlternate(const mkuint32 alternatesIdx) const
        { return GetAlternates(alternatesIdx)->GetMatchedAlternate(); }

        void AddMolecule(const std::shared_ptr<Molecule>& rMolecule)
        { m_molecules.push_back(rMolecule); }

        mkuint32 MoleculeCount() const
        { return (mkuint32)m_molecules.size(); }

    private:
        // Helper methods for marching through molecules.
        MatchResult AdvanceMatch(const MkAString& string, mkuint32& charIdx);
        MatchResult RetreatMatch(const MkAString& string);

        friend class MkStringPattern;
        bool Parse(const MkAString& metaPattern, mkuint32& charIdx, const MkAString& endChars);

        std::vector<std::shared_ptr<Molecule>> m_molecules;
        mkuint32 m_moleculeIdx;

        mkuint m_tag;
    };


    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Alternates
    ////////////////////////////////////////////////////////////////////////////////////////////////
    
    // Molecules evaluated as alternates.
    class Alternates : public Molecule
    {
    public:
        Alternates() :
            Molecule(eAlternates)
        { }

        MatchResult Match(const MkAString& string, mkuint32 charIdx) override;

        MatchResult NextMatch(const MkAString& string) override;

        void AddAlternate(const Sequence& sequence)
        { m_alternates.push_back(sequence); }

        mkuint32 GetCount() const
        { return (mkuint32)m_alternates.size(); }

        mkuint32 GetAlternateIndex() const
        { return m_alternateIdx; }

        const Sequence& GetMatchedAlternate() const
        { return m_alternates[m_alternateIdx]; }

        static const std::shared_ptr<Molecule> Parse(
                const MkAString& metaPattern, mkuint32& charIdx, const MkAString& endChars);

    private:
        // NOTE: No protection against invalid requests.  MRK.
        Sequence& GetAlternate(const mkuint32 alternateIdx)
        { return m_alternates[alternateIdx]; }

        std::vector<Sequence> m_alternates;
        mkuint32 m_alternateIdx = 0;

        // The character at which the first attempt to match alternates occurred.
        mkuint32 m_beginCharIdx = 0;
    };

public:
    MK_EXPORT_FOUNDATION MkStringPattern(const MkAString& pattern);

    MK_EXPORT_FOUNDATION MkStringPattern(const Sequence& sequence) :
        m_moleculeSequence(sequence)
    { }

    // Check if the pattern is a happy pattern.
    bool IsValid() const
    { return bool(m_moleculeSequence.MoleculeCount() > 0);}

    // Do the actual match of a string.
    bool Match(const MkAString& string, const mkuint32 startIdx = 0)
    { return m_moleculeSequence.Match(string, startIdx).m_matched; }

    // Fetch relevant molecules for match information.  Don't enter alternates.
    // NOTE: No protection against invalid requests.  MRK.
    const std::shared_ptr<Marker> GetMarker(const mkuint32 markerIdx) const
    { return std::static_pointer_cast<Marker>(m_moleculeSequence.GetMarker(markerIdx)); }

    const std::shared_ptr<Alternates> GetAlternates(const mkuint32 alternatesIdx)
    { return std::static_pointer_cast<Alternates>(m_moleculeSequence.GetAlternates(alternatesIdx)); }

    const Sequence&  GetMatchedAlternate(const mkuint32 alternatesIdx) const
    { return m_moleculeSequence.GetAlternates(alternatesIdx)->GetMatchedAlternate(); }

    enum KeyCharacter : mkchar8
    {
        eQuestionMark = 1,
        eOpenBracket,
        eCloseBracket,
        eHyphen,
        eTilde,
        eAsterisk,
        ePlus,
        eOpenBrace,
        eVerticalBar,
        eCloseBrace,
        ePerCent,
        eDollarSign,
        eApostrophe,
        eAtSign,
        ePatternEnd
    };

private:
    MkAString TranslatePattern(const MkAString& pattern) const;

    // Helper method.
    static bool PatternEnd(const MkAString& metaPattern, mkuint32 charIdx, const MkAString& endChars)
    { return bool(endChars.Find(metaPattern[charIdx]) >= 0); }

    Sequence m_moleculeSequence;
};