-- TODO: zkontrolovat jesli u vsech funkci osetrujes vsechny stavy
--
import System.Environment
import System.IO
import Data.List

main = do
  args <- getArgs
  progName <- getProgName

  -- budeme cist ze souboru nebo z stdin?
  let source = if ((length args) > 1) then (last args)  else ""
  fileContent <- getFileContent source
  let fileContentList = (wordsWhen (=='\n')) fileContent

  -- ziskani jednotlivych radku ze zadaneho souboru
  let lineNonTerms  = head fileContentList
  let lineTerms     = head $ drop 1 fileContentList
  let lineStart     = head $ drop 2 fileContentList
  let lineRules     = drop 3 fileContentList

  -- vytvoreni prave linearni gramatiky
  let plg = Gramatika (wordsWhen (==',') lineNonTerms) (wordsWhen (==',') lineTerms) (filePlgRules lineRules) (head lineStart)

  case (head args) of "-i" -> printGrammar plg
                      "-1" -> printGrammar (getPrg plg)
                      "-2" -> printNka (getPrg plg)
                      _    -> error "Spatne specifikovane vstupni argumenty"
 
-- precteni vstupni gramatiky ze souboru nebo stdin v zavislosti na parametru,
-- kterym je jmeno souboru
getFileContent :: [Char] -> IO String 
getFileContent "" = getContents
getFileContent x  = readFile (x)

-- =============================================================================
-- FUNKCE ZAJISTUJICI TISK GRAMATIKY NEBO NKA NA STDOUT:
-- =============================================================================
-- Tisk gramatiky G = (N,T,P,S):
printGrammar g = do
  putStrLn (intercalate "," (nonTerms g))                       -- N
  putStrLn (intercalate "," (terms g))                          -- T
  putStrLn [(start g)]                                          -- P
  mapM_ (\f -> putStr (fst f++"->"++snd f++"\n")) (rules g) -- S

-- Tisk NKA M = (Q,N,Delta,q0,F):
printNka g = do
  let nka = getNka g
  putStrLn (intercalate "," (states nka))
  putStrLn [q0 nka]
  putStrLn (intercalate "," (final nka))
  mapM_ (\f -> putStr ((fst $ fst f)++","++[snd $ fst f]++","++snd f++"\n")) (fce nka)


-- =============================================================================
-- Definice noveho datoveho typu reprezentujici gramatiku G = (N, T, P, S).
-- =============================================================================
data Gramatika = Gramatika { nonTerms :: [String]           -- N
                           , terms    :: [String]           -- T
                           , rules    :: [(String, String)] -- P
                           , start    :: Char               -- S
                           } deriving (Show)

isNonTerm = (`elem` ['A'..'Z'])   -- je dany znak nonterminal?
isTerm = (`elem` ['a'..'z'])      -- je dany znak terminal?

-- Zadany retezec podle zadaneho rozdelovace rozseka do pole retezcu
wordsWhen :: (Char -> Bool) -> String -> [String]
wordsWhen p s = 
  case dropWhile p s of
    "" -> []
    s' -> w : wordsWhen p s'' where (w, s'') = break p s'

filePlgRules :: [String] -> [(String, String)]
filePlgRules rules = map (\r -> ([head r], (drop 3 r))) rules


--testovaci gramatika
--plg = Gramatika ["S","A", "C"] ["a", "b"] [("S", "A"), ("A", "abC"), ("C", "A"), ("A", "#")] 'S'
 
-- =============================================================================
-- NEDETERMINISTICKY KONECNY AUTOMAT Fsm = (Q, sigma, delta, q0, F)
-- =============================================================================
data Fsm = Fsm { states   :: [String]                   -- Q
               , alphabet :: [String]                   -- sigma
               , fce      :: [(([Char], Char), [Char])] -- delta
               , q0       :: Char                       -- qO
               , final    :: [String]                   -- F
               } deriving (Show)

-- =============================================================================
-- PREVOD PRAVE LINEARNI GRAMATIKY NA PRAVOU REGULARNI GRAMATIKU:
-- =============================================================================
--
-- Tyto funkce vyfiltruji pravidla daneho typu z prave linearni gramatiky
--------------------------------------------------------------------------------
-- Je dane pravidlo epsilon pravidlem?
isEpsRule :: (String, String) -> Bool
isEpsRule = (\r -> (isSimpleNonTerm $ fst(r)) && (snd(r) == "#"))

-- Je pravidlo typu A->a1a2...anB, kde n>=2?
isNontermRule :: (String, String) -> Bool
isNontermRule = (\r -> (isNonTerm $ last $ snd r) && ((length $ snd r) >= 3))

-- 1.  pravidla tvaru A -> aB, kde 'a' je term a 'B' je nonterm
getPlgRuleT1 :: Gramatika -> [(String, String)]
getPlgRuleT1 g = filter (\r -> isTerm (head $ snd r) && isNonTerm (last $ snd r) && (length $ snd r) == 2) (rules g)

-- 11.   epsilon pravidla, tj. pravidla tvaru A -> eps
getPlgRuleT11 :: Gramatika -> [(String, String)]
getPlgRuleT11 g = filter isEpsRule (rules g)

-- 2.   pravidla tvaru A->a1a2..anB, kde n>=2 a B je nonterm
getPlgRuleT2 :: Gramatika -> [(String, String)]
getPlgRuleT2 g = filter isNontermRule (rules g)

-- 3.   pravidla tvaru A->a1a2..an, kde n>=1
getPlgRuleT3 :: Gramatika -> [(String, String)]
getPlgRuleT3 g = filter (\r -> not (isNonTerm $ last $ snd r) && (last $ snd r) /= '#') (rules g)

-- 4.   jednoducha pravidla tvaru A->B, kde A i B jsou nonterminaly
getPlgRuleT4 :: Gramatika -> [(String, String)]
getPlgRuleT4 g = filter (\r -> length (snd r) == 1 && isNonTerm (head $ snd r)) (rules g)

--------------------------------------------------------------------------------
-- Nasledujici funkce provadeji prevod pravidel prave linearni gramatiky na
-- pravidla odpovidajici prave regularni gramatice. Zejmena prevod pravidel
-- typu:  A -> a1a2...anB, n>=2 a
--        A -> a1a2...an, n>=1.
--------------------------------------------------------------------------------
-- Tato funkce dostane jako parametr pole jiz prevedenych pravidel typu 2
-- linearni gramatiky a vrati polsedni pouzity index nonterminalu. Napr.  polem
-- pravidel [("A", "aA1"), ("A1", "bA2), ("A2", "dC")] by byl posledni pouzity
-- index nonterminalu 2.
getLastIndex :: [(String, String)] -> Int
getLastIndex [] = 1
getLastIndex x = read (drop 1 $ fst $ last x) :: Int

-- Pokud byl predchozi nonterminal (promenna l) bez cisla na konci, napr. A, tak
-- dalsi bude A1. Pokud byl predchozi nonterminal napr. A4, pak bude dalsi A5.
newNonTerm l i = if (length l == 1) then l++(show i) else head l:(show i)

-- Tato funkce provede prevod pravidla linearni gramatiky typu A->abdC na
-- pravidla A->aA1, A1->bA2, A2->dC, coz by bylo reprezenovatno polem dvojic
-- [("A", "aA1"), ("A1", "bA2), ("A2", "dC")]. Prvnim parametrem je index,
-- kterym zacnou byt indexovany nove neterminaly.
convertRuleT2 :: (Show a, Num a) => a -> (String, String) -> [(String, String)]
convertRuleT2 i (l, (an:b:[])) = [(l, an:[b])]
convertRuleT2 i (l, (a1:as)) = (l, a1:(newNonTerm l i)):(convertRuleT2 (i+1) ((newNonTerm l i), as))

-- Tato funkce dostane seznam pravidel typu:
--    A -> a1a2...anB, n>=2
-- a kazde takove pravidlo prevede na pravidla typu:
--    A    -> a1A1
--    A1   -> a2A2
--    ...
--    An-1 -> anB
--convertPlgRulesType2 :: Int -> [(String), (String)] -> [(String), (String)]
convertPlgRulesType2 _ [] = []
convertPlgRulesType2 n (x:xs) = listOfNewRules++convertPlgRulesType2 newStartIndex xs
  where listOfNewRules = convertRuleT2 n x
        newStartIndex = (getLastIndex listOfNewRules) + 1

-- prevod pravidlo typu
--    A -> a1a2...an, n>=1 
-- na seznam pravidel:
--    A    -> a1A1
--    A1   -> a2A2
--    ...
--    An-1 -> anAn
--    An   -> eps
sndCase :: (Show a, Num a) => a -> (String, String) -> [(String, String)]
sndCase i (l, [a]) = [(l, [a]++(newNonTerm l i)), ((newNonTerm l i), "#")]
sndCase i (l, (a1:as)) = (l, a1:(newNonTerm l i)):(sndCase (i+1) ((newNonTerm l i), as))

-- prevede seznam pravidel typu A -> a1a2...an, n>=1, ktere jsou reprezentovany
-- dvojcemi [("A", "a1a2...an")]
convertPlgRulesType3 _ [] = []
convertPlgRulesType3 n (x:xs) = listOfNewRules++convertPlgRulesType3 newStartIndex xs
  where listOfNewRules = sndCase n x
        newStartIndex = (getLastIndex listOfNewRules) + 1

-- Vytvoreni nove mnoziny pravidel, ktera odpovida pravidlum prave regularni
-- gramatiky. Pri prevodu pravidel typu pouzivam index, ktery byl naposledy
-- pouzit pri prevodu pravidel typu 2.
prgRules g = rt1++rt11++(convertPlgRulesType2 1 (getPlgRuleT2 g))++convertPlgRulesType3 nextIndex (getPlgRuleT3 g)++(getPlgRuleT4 g)
  where nextIndex = getLastIndex(convertPlgRulesType2 1 (getPlgRuleT2 g))+1
        rt1 = (getPlgRuleT1 g)  -- A -> aB
        rt11 = (getPlgRuleT11 g)  --A -> eps

-- Vysledna prava regularni gramatika (nub odstrani duplicitni nonterminaly a
-- terminaly).
getPrg g = Gramatika (nub ((map fst (prgRules g))++(nonTerms g))) (nub (terms g)) (removeSimpleRules (prgRules g)) (start g)
-- =============================================================================

-- =============================================================================
-- PREVOD PRAVE REGULARNI GRAMATIKY NA NEDETERMINISTICKY KONECNY AUTOMAT:
-- =============================================================================
-- Vezme mnozinu pravidel prave regularni gramatiky typu A->aB, kde A,B jsou
-- neterminaly a 'a' je terminal a prevede ji na prechodovou funkci.
getNka g =
  let nkaDelta = map (\i -> ((fst i, head $ snd i), drop 1 $ snd i)) (filter (\r -> (length $ snd r) >1) (rules g))
      nkaFinite = map (\i -> fst i)  (filter (\r -> snd(r)=="#") (rules g))
  in Fsm (nonTerms g) (terms g) nkaDelta (start g) nkaFinite

-- =============================================================================
-- ODSTRANENI JEDNODUCHCH PRAVIDEL
-- =============================================================================
--p = [("A","aZ"),("A","bC"),("A","B"),("B","aZ"),("B","bC"),("B","D"),("D","#")]
p = [("A","a")]
-- Je rezetec tvoren pouze neterminalnim symbolem?
isSimpleNonTerm :: String -> Bool
isSimpleNonTerm = (\r -> (length r == 1) && (isNonTerm (head r))) 

-- Je pravidlo jednoduchym pravidlem, tj. je prava i leva strana tvorena pouze
-- nonterminalnim symbolem?
isRuleSimple :: (String, String) -> Bool
isRuleSimple = (\r -> (isSimpleNonTerm $ fst(r)) && (isSimpleNonTerm $ snd(r)))

-- Vezme mnozinu pravidel a vrati seznam vsech jednoduchych pravidel.
getSimpleRules :: [(String, String)] -> [(String, String)]
getSimpleRules [] = []
getSimpleRules rules = filter (\r -> isRuleSimple r) rules

-- Vrati mnozinu pravidel, ze ktere odstrani zadane jednoduche pravidlo.
withouSimpleRule :: (String, String) -> [(String, String)] -> [(String, String)]
withouSimpleRule simple rules = filter ((\s r -> (fst(s) /= fst(r) || snd(s) /= snd(r))) simple) rules

-- Vstupem je jednoduche pravidlo a mnozina vsech pravidel gramatiky.  Pak tato
-- funkce pro jednoduche pravidlo A->B vrati nejednoduche prave strany vsech
-- pravidel, ktere maji vlevo terminal B. Napr pro jednoduche pravidlo A->B, a
-- pravidla B->aZ|bC|D, vrati [aZ,bC].
-- FIXME: uprav to trosku
getNotSimpleRighSides :: (String, String) -> [(String, String)] -> [String]
getNotSimpleRighSides _ [] = []
getNotSimpleRighSides (l,t) (x:xs) = 
  if fst(x) == t && (epsRule || nonSimple) then
    [snd(x)]++getNotSimpleRighSides (l,t) xs
    else
    getNotSimpleRighSides (l,t) xs
  where epsRule = isEpsRule x
        nonSimple = ((length $ snd(x)) >= 2)  -- pravidlo typu 2 nebo 3

-- Vezme neterminal a mnozinu pravych stran a vytvori nove pravidla. Pr. pro
-- neterminal A a prave strany ["aB", "cZ"] vytvori pravidla: A->aB a A->cZ.
getNewRules :: String -> [String] -> [(String, String)]
getNewRules _ [] = []
getNewRules t (x:xs) = [(t, x)]++getNewRules t xs

-- Tato funkce dostane mnozinu pravidel a jednoduche pravidlo a toto jednoduche
-- pravidlo nahradi tak, ze prave strany toho pravidla nahradi pravymi stranami
-- pravidel, ktere maji vlevo stejny neterminal jako ma jednoduche pravidlo
-- vpravo. Priklad: Pro pravidla:
--  A->B
--  B->aC|aZ
-- dostanu pravidla:
--  A->aC|aZ
--  B->aC|aZ
replaceSimpleRule :: (String, String) -> [(String, String)] -> [(String, String)]
replaceSimpleRule simple rules = 
  withouSimpleRule simple rules++(getNewRules (fst(simple)) (getNotSimpleRighSides simple rules))

-- Tato funkce odstrani z mnoziny pravidel vsechna jednoducha pravidla. Vyuziva
-- operatoru pevneho bodu kdy kazde jednoduche pravidlo nahradi novymi pravidly,
-- tak dlouho dokud se v mnozine pravidel nachazi nejaka jednoducha pravidla.
removeSimpleRules :: [(String, String)] -> [(String, String)]
removeSimpleRules [] = []
removeSimpleRules r =
  let containSimpleRule = not (null (getSimpleRules r))
  in  if (containSimpleRule) 
      then removeSimpleRules (replaceSimpleRule (head (getSimpleRules r)) r) 
      else r
