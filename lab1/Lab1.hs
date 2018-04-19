module Lab1 where
import Prelude hiding (Num, seq)

-----

-- 1a. define abstract syntax for logo

data Command = Pen Mode
             | MoveTo Pos Pos
             | Def Name Params Command
             | Call Name Vals
             | Seq Command Command       deriving (Show)
data Mode = Up | Down                    deriving (Show)
data Pos = Num Int | Name String         deriving (Show)
data Params = Params [Name]              deriving (Show)
data Vals = Vals [Num]                   deriving (Show)
type Name = String
type Num = Int

-- 1b. define, as an ast, a macro for logo which draws a vector

-- def vector(x1, y1, x2, y2)
--    pen up
--    moveto(x1, y1)
--    pen down
--    moveto(x2, y2)

defVector :: Command
defVector = Def "vector" (Params ["x1", "y1", "x2", "y2"])
                ( Seq (Pen Up)
                $ Seq (MoveTo (Name "x1") (Name "y1"))
                $ Seq (Pen Down)
                $     (MoveTo (Name "x2") (Name "y2"))
                )

-- 1c. define a haskell function steps which generates a program which draws n stair steps

seq :: [Command] -> Command
seq = foldr1 Seq

steps :: Int -> Command
steps n =  seq $ map step [0..n-1]
    where step n = seq [ Pen Up
                        , MoveTo (Num n) (Num n)
                        , Pen Down
                        , MoveTo (Num n) (Num (n+1))
                        , MoveTo (Num (n+1)) (Num (n+1))
                        ]

main = print $ steps 1

-----------


-- 2a. define abstract syntax


data Circuit = Circuit Gates Links deriving (Show)
data Gates = Gates [(Int, GateFn)] deriving (Show)
data GateFn = Xor | And | Or | Not deriving (Show)
data Links = Links [Link] deriving (Show)
data Link = Link Coord Coord deriving (Show)
type Coord = (Int, Int)

-- 2b. represent half adder circuit

circuit = Circuit (Gates [ (1, Xor), (2, And) ])
                  (Links [ Link (1,1) (2,1)
                         , Link (1,2) (2,2) ])

-- 2c. define a pretty-printer
-- TODO

--main = print circuit
