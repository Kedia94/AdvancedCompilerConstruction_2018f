//
// test05
//
// computations with integers arrays
//
// expected output: 1987654321 (no newline)
//

module test14;

var input : integer;
    i : integer;
	constant : integer;
	iter : integer;
	iter2 : integer;

function sum_rec(n: integer): integer;
begin
  if (n > 0) then
    return i + sum_rec(n-1)
  else
    return 0
  end
end sum_rec;

function sum_iter(n: integer): integer;
var sum, i: integer;
begin
  sum := 0;
  i := 0;
  while (i <= n) do
    sum := sum + i;
    i := i+1
  end;
  return sum
end sum_iter;

begin
  iter := 100000000;
  iter2 := 100000000;

  while (iter > 0) do
  input := 10;
  i := 1;
  constant := 1;
  while (i>0) do
    input := constant + 1;
    if (constant > 0) then
      sum_rec(i)
	else
	  sum_iter(i)
	end;
    i := i-1
  end;
iter := iter-1
  end
end test14.
