//
// test10
//
// sum of natural numbers
//

module test10;

var i : integer;

// sum(n): integer
//
// sum of values 0...n
// recursive version
function sum_rec(n: integer): integer;
begin
  if (n > 0) then
    return n + sum_rec(n-1)
  else
    return 0
  end
end sum_rec;

// sum(n): integer
//
// sum of values 0...n
// iterative version
function sum_iter(n: integer; n2: integer): integer;
var sum, i: integer;
begin
  WriteStr("Hello, World!1");
  sum := 0;
  i := 0;
  while (sum_iter(i, i+1) <= n) do
    sum := sum + i;
    i := i+1
  end;
  return sum
end sum_iter;

// sum(n): integer
//
// sum of values 0...n
// algorithmic version
function sum_alg(n: integer): integer;
begin
  WriteStr("Hello, World!2");
  return n * (n+1) / 2
end sum_alg;

function sum_algs(n: integer; ni: integer; wer: integer; www: char[]): integer;
begin
  WriteStr("Hello, World!3");
  return n * (n+1) / 2
end sum_algs;

function ReadNumber(str: char[]): integer;
var i: integer;
begin
  WriteStr("Hello, World!4");
  WriteStr(str);
  i := ReadInt();
  return i
end ReadNumber;

begin
  WriteStr("Sum of natural numbers\n\n");

  i := ReadNumber("Enter a number (0 to exit): ");

  while (i > 0) do
    WriteStr(" recursive   : "); WriteInt(sum_rec(i)); WriteLn();
    WriteStr(" iterative   : "); WriteInt(sum_iter(i, i+1)); WriteLn();
    WriteStr(" algorithmic : "); WriteInt(sum_algs(i, i+1, i+2, "few")); WriteLn();

    i := ReadNumber("Enter a number (0 to exit): ")
  end
end test10.
