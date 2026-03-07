```javascript [app.js] {1-3,5}
const x = 1;
const y = 2;
const z = 3;
console.log(x);
console.log(y);
```

```ts [@[...slug].ts] {2}
export default function handler(req, res) {
  res.json({ slug: req.params.slug });
}
```

```python {1,3-5}
import os
import sys
def main():
    print("hello")
    return 0
```
