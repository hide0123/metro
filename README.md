自作スクリプト言語

静的型付け


サンプル:
```
fn fibo(n: int) -> int {
  if n < 2 {
    1
  }
  else {
    fibo(n - 2) + fibo(n - 1)
  }
}

fn main() {
  println(fibo(10))
}
```

