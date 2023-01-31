自作言語
スクリプト言語

C にトランスパイルされてバイナリにはなるが、内部挙動はスクリプト言語と同じ
つまり Objective-C

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

