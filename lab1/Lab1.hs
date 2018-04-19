module Lab1 where

data Command = Pen Mode
             | MoveTo Pos Pos
             | Def Name Params Command
             | Call Name Vals
             | Seq Command Command
data Mode = Up | Down
data Pos = Num Int | Name String
data Params = Params [Name]
data Vals = Vals [Num]

