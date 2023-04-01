# About

Metro is a scripting language that uses Rust-like syntax, but is simpler than Rust and can be executed easily as a script. Additionally, it is written entirely in C++ without using any libraries such as Lex or Yacc.

[Repository](https//github.com/bomkei/metro)
[Documentation](https//github.com/bomkei/metro-docs)

# Build & HelloWorld

```bash
git clone https://github.com/bomkei/metro.git
cd metro
make release -j
./metro -c "println(\"Hello, World!\");"
```

# Sample Code

For language specification, please refer to [metro-docs/language.md](https://github.com/bomkei/metro-docs/blob/main/language.md).

## Hello World
Since there is no need for a `main` function, it can be written in one line.

```
println("Hello, World!");
```

## Defining Variables
You can define variables using the let statement. Shadowing is also possible.

```
let num = 123;
let num = "num is " + to_string(num);

println(num); // => num is 123
```

## Defining Functions

Example of a function that calculates the Fibonacci number.

```
fn fibo(n: int) -> int {
  if n < 2 {
    return 1;
  }

  fibo(n - 2) + fibo(n - 1)
}

println(fibo(10)); // => 89
```

## Structs, Enumerations

An example of initializing an object of the structure Person with an initializer using the new keyword and displaying it. Enumeration elements can have values of any type.

```
enum Gender {
  Red,
  Blue,
  Other(string)
}

struct Person {
  name: string,
  gender: Gender,
  age: float
}

let person = new Person(name: "letz",
                        gender: Gender.Blue,
                        age: 17.5);

println(person);
  // => Person{ name: "letz", gender: Gender.Blue, age: 17.5 }
```

## Extension Functions
You can add extension functions to any type using the impl syntax. Extension functions can be called using the same syntax as member functions.

Below is an example of defining and calling the "twice" extension function for `int`.

```
impl int {
  fn twice(self) -> int {
    self * 2
  }
}

pritnln(10.twice()); // => 20
```
