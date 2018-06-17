# exprc
A toy HLS compiler written in modern C++

---
## Outline
An "write my very own HLS compiler over weekend" effort, which took me more than
single weekend, and ended in building a very simple HLS tool,
which translates [this](#this) into [that](#that)

## Limitations

Due to time limitations, the tool tends to do simplest thing instead of
doing right one.

* Uses `8-bit` for all operands, results and intrernal computations
* Supports only two arithmetic instructions: `+` and `*`
* Does not support any control flow capabilities at all
* Usage of resources is not optimal:
  * Greedy executes as much operations as possible in the earliest possible control step
  * Does not aware of interconnect between registers / execution units

## Usage

### Language

**Exprc** accepts program of below form as input and outputs verilog module into `stdout`
```
Program    -> { Assignment ';' | 'out' Assignment ';' }
Assignment -> Variable = Expression
Expression -> Term { '+' Term }*
Term       -> Factor { '*' Factor }*
Factor     -> Variable | '(' E ')'
```

Each variable must be assigned only once.

Assignments prefixed with `out` are supposed to become output of generated module.

Results of non `out` assignments must contribute into results of `out` ones. That is
here must be direct (or transitive) data dependency between them and `out` assignments.

Program which does not satisfy any of these criteria will be rejected as incorrect.

### Build

```
$ mkdir build
$ cd build
$ cmake <path/to/cloned/exprc/repo>
$ make -j
```

#### Dependencies

* [fmtlib](https://github.com/fmtlib/fmt)
* [cmake](https://cmake.org) `>= 3.8`
* compiler with `c++17` support (`g++ 7.1.0` was used while development)

### Example

#### This
```
A = B + C;
D = E + F;
out G = D + A;
out H = D * A;
```

#### That
![wave](media/wave.png?raw=true "wave")

```verilog
module exprc(
  input wire clk,
  input wire rst,
  input wire ena,
  input wire [7:0] B,
  input wire [7:0] C,
  input wire [7:0] E,
  input wire [7:0] F,
  output wire [7:0] G,
  output wire [7:0] H,
  output reg done,
  output reg ready
);

  localparam [0:1]
    S1 = 2'd0,
    S2 = 2'd1;
  reg [7:0] reg2;
  reg [7:0] reg0;
  reg [7:0] reg1;

  reg [7:0] add7_in3;
  reg [7:0] add7_in4;
  wire [7:0] add7 = add7_in3 + add7_in4;

  reg [7:0] add8_in5;
  reg [7:0] add8_in6;
  wire [7:0] add8 = add8_in5 + add8_in6;

  reg [7:0] mul9_in7;
  reg [7:0] mul9_in8;
  wire [7:0] mul9 = mul9_in7 * mul9_in8;

  assign G = reg0;
  assign H = reg1;

  reg [0:1] state;
  always @(posedge clk)
    begin
      if (rst)
        begin
          state <= S1;
          done <= 1'b0;
          ready <= 1'b1;
        end
    else
      begin
        case (state)
          S1:
            begin
              if (ena)
                begin
                  state <= S2;
                  done <= 1'b0;
                  ready <= 1'b0;
                end
              reg0 <= add8;
              reg2 <= add7;
            end
          S2:
            begin
              state <= S1;
              reg0 <= add7;
              reg1 <= mul9;
              done <= 1'b1;
              ready <= 1'b1;
            end
        endcase
      end
    end

  always @(*)
    begin
      case (state)
        S1:
          begin
            add7_in3 = B;
            add7_in4 = C;
            add8_in5 = E;
            add8_in6 = F;
            mul9_in7 = 8'dX;
            mul9_in8 = 8'dX;
          end
        S2:
          begin
            add7_in3 = reg0;
            add7_in4 = reg2;
            mul9_in7 = reg0;
            mul9_in8 = reg2;
            add8_in5 = 8'dX;
            add8_in6 = 8'dX;
          end
      endcase
    end

endmodule
```
