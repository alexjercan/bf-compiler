const std = @import("std");

fn readFile(file_name: ?[]u8) anyerror![]u8 {
    var in_file: ?std.fs.File = null;

    if (file_name == null) {
        in_file = std.io.getStdIn();
    } else {
        in_file = try std.fs.cwd().openFile(file_name.?, .{});
    }

    const stdin_file = in_file.?.reader();

    var br = std.io.bufferedReader(stdin_file);
    const stdin = br.reader();

    var buffer = [_]u8{0} ** 1024;
    var size = try stdin.readAll(&buffer);

    var text = std.ArrayList(u8).init(std.heap.page_allocator);
    try text.appendSlice(buffer[0..size]);

    while (size >= 1024) {
        buffer = [_]u8{0} ** 1024;
        size = try stdin.readAll(&buffer);

        try text.appendSlice(buffer[0..size]);
    }

    if (file_name != null) {
        in_file.?.close();
    }

    return text.toOwnedSlice();
}

fn writeFile(file_name: ?[]u8, buffer: []u8) anyerror!void {
    var out_file: ?std.fs.File = null;

    if (file_name == null) {
        out_file = std.io.getStdOut();
    } else {
        out_file = try std.fs.cwd().createFile(file_name.?, .{});
    }

    const stdout_file = out_file.?.writer();

    var bw = std.io.bufferedWriter(stdout_file);
    const stdout = bw.writer();

    try stdout.writeAll(buffer);

    try bw.flush();

    if (file_name != null) {
        out_file.?.close();
    }
}

const MyArgs = struct { input: ?[]u8, output: ?[]u8 };

fn parseArgs(args: *std.process.ArgIterator) anyerror!MyArgs {
    var result: MyArgs = .{ .input = null, .output = null };
    while (args.next()) |arg| {
        if (std.mem.eql(u8, arg, "-f") or std.mem.eql(u8, arg, "--file")) {
            if (args.next()) |input| {
                result.input = try std.heap.page_allocator.dupeZ(u8, input);
            }
        }
        if (std.mem.eql(u8, arg, "-o") or std.mem.eql(u8, arg, "--output")) {
            if (args.next()) |output| {
                result.output = try std.heap.page_allocator.dupeZ(u8, output);
            }
        }
    }

    return result;
}

const TokenKind = enum(u8) {
    move_right,
    move_left,
    increment,
    decrement,
    output,
    input,
    jump_forward,
    jump_backward,
    comment,
};

fn instructionsParse(program: []u8) anyerror![]TokenKind {
    var tokens = std.ArrayList(TokenKind).init(std.heap.page_allocator);

    for (program) |token| {
        const kind = switch (token) {
            '>' => TokenKind.move_right,
            '<' => TokenKind.move_left,
            '+' => TokenKind.increment,
            '-' => TokenKind.decrement,
            '.' => TokenKind.output,
            ',' => TokenKind.input,
            '[' => TokenKind.jump_forward,
            ']' => TokenKind.jump_backward,
            else => TokenKind.comment,
        };

        try tokens.append(kind);
    }

    return try tokens.toOwnedSlice();
}

const MatchingBracketsTag = enum { ok, not_ok_jf, not_ok_jb };

const MatchingBrackets = union(MatchingBracketsTag) {
    ok: void,
    not_ok_jf: usize,
    not_ok_jb: usize,
};

fn matching_brackets(instructions: []TokenKind) anyerror!MatchingBrackets {
    var counter = std.ArrayList(usize).init(std.heap.page_allocator);
    defer counter.deinit();

    for (instructions, 0..) |kind, i| {
        if (kind == TokenKind.jump_forward) {
            try counter.append(i);
        } else if (kind == TokenKind.jump_backward) {
            if (counter.popOrNull()) |_| {} else {
                return MatchingBrackets{ .not_ok_jb = i };
            }
        }
    }

    if (counter.getLastOrNull()) |_| {
        return MatchingBrackets{ .not_ok_jf = (try counter.toOwnedSlice())[0] };
    }

    return MatchingBrackets.ok;
}

fn display_error(name: ?[]u8, program: []u8, index: usize, desc: []const u8) void {
    var line: usize = 1;
    var col: usize = 1;

    for (0..index) |i| {
        if (program[i] == '\n') {
            line = line + 1;
            col = 1;
        } else {
            col = col + 1;
        }
    }

    if (name != null) {
        std.debug.print("{s}:{d}:{d}: {s}\n", .{ name.?, line, col, desc });
    } else {
        std.debug.print("{d}:{d}: {s}\n", .{ line, col, desc });
    }
}

fn generate_assembly_move_right(buffer: *std.ArrayList(u8), count: usize) anyerror!void {
    if (count == 1) {
        try buffer.appendSlice("    inc qword [pointer]         ; increment pointer to current cell\n");
    } else {
        try buffer.writer().print("    add qword [pointer], {d}\n", .{count});
        try buffer.appendSlice("                                ; increment pointer to current cell\n");
    }
}

fn generate_assembly_move_left(buffer: *std.ArrayList(u8), count: usize) anyerror!void {
    if (count == 1) {
        try buffer.appendSlice("    dec qword [pointer]         ; increment pointer to current cell\n");
    } else {
        try buffer.writer().print("    sub qword [pointer], {d}\n", .{count});
        try buffer.appendSlice("                                ; increment pointer to current cell\n");
    }
}

fn generate_assembly_increment(buffer: *std.ArrayList(u8), count: usize) anyerror!void {
    if (count == 1) {
        try buffer.appendSlice("    mov rax, [pointer]          ; move pointer to current cell to rax\n");
        try buffer.appendSlice("    inc byte [rax]              ; increment current cell\n");
    } else {
        try buffer.appendSlice("    mov rax, [pointer]          ; move pointer to current cell to rax\n");
        try buffer.writer().print("    add byte [rax], {d}\n", .{count});
        try buffer.appendSlice("                                ; increment current cell\n");
    }
}

fn generate_assembly_decrement(buffer: *std.ArrayList(u8), count: usize) anyerror!void {
    if (count == 1) {
        try buffer.appendSlice("    mov rax, [pointer]          ; move pointer to current cell to rax\n");
        try buffer.appendSlice("    dec byte [rax]              ; increment current cell\n");
    } else {
        try buffer.appendSlice("    mov rax, [pointer]          ; move pointer to current cell to rax\n");
        try buffer.writer().print("    sub byte [rax], {d}\n", .{count});
        try buffer.appendSlice("                                ; increment current cell\n");
    }
}

fn generate_assembly_output(buffer: *std.ArrayList(u8)) anyerror!void {
    try buffer.appendSlice("    mov rax, 1                  ; system call for write\n");
    try buffer.appendSlice("    mov rdi, 1                  ; stdout\n");
    try buffer.appendSlice("    mov rsi, [pointer]          ; pointer to current cell\n");
    try buffer.appendSlice("    mov rdx, 1                  ; length\n");
    try buffer.appendSlice("    syscall                     ; invoke operating system to read\n");
}

fn generate_assembly_input(buffer: *std.ArrayList(u8)) anyerror!void {
    try buffer.appendSlice("    mov rax, 0                  ; system call for read\n");
    try buffer.appendSlice("    mov rdi, 0                  ; stdin\n");
    try buffer.appendSlice("    mov rsi, [pointer]          ; pointer to current cell\n");
    try buffer.appendSlice("    mov rdx, 1                  ; length\n");
    try buffer.appendSlice("    syscall                     ; invoke operating system to read\n");
}

fn generate_assembly_jump_forward(buffer: *std.ArrayList(u8), label: usize) anyerror!void {
    try buffer.appendSlice("    mov rax, [pointer]          ; move pointer to current cell to rax\n");
    try buffer.appendSlice("    cmp byte [rax], 0           ; compare current cell to 0\n");
    try buffer.writer().print("    je .LB{d}\n", .{label});
    try buffer.appendSlice("                                ; jump to closing bracket if equal\n");
    try buffer.writer().print(".LF{d}:\n", .{label});
}

fn generate_assembly_jump_backward(buffer: *std.ArrayList(u8), label: usize) anyerror!void {
    try buffer.appendSlice("    mov rax, [pointer]          ; move pointer to current cell to rax\n");
    try buffer.appendSlice("    cmp byte [rax], 0           ; compare current cell to 0\n");
    try buffer.writer().print("    jne .LF{d}\n", .{label});
    try buffer.appendSlice("                                ; jump to opening bracket if not equal\n");
    try buffer.writer().print(".LB{d}:\n", .{label});
}

fn generate_assembly_block(buffer: *std.ArrayList(u8), instructions: []TokenKind, index: *usize) anyerror!void {
    while (index.* < instructions.len) {
        const kind = instructions[index.*];
        switch (kind) {
            TokenKind.move_right => {
                var count: usize = 1;
                while (index.* < instructions.len - 1 and instructions[index.* + 1] == TokenKind.move_right) {
                    count = count + 1;
                    index.* = index.* + 1;
                }
                try generate_assembly_move_right(buffer, count);
            },
            TokenKind.move_left => {
                var count: usize = 1;
                while (index.* < instructions.len - 1 and instructions[index.* + 1] == TokenKind.move_left) {
                    count = count + 1;
                    index.* = index.* + 1;
                }
                try generate_assembly_move_left(buffer, count);
            },
            TokenKind.increment => {
                var count: usize = 1;
                while (index.* < instructions.len - 1 and instructions[index.* + 1] == TokenKind.increment) {
                    count = count + 1;
                    index.* = index.* + 1;
                }
                try generate_assembly_increment(buffer, count);
            },
            TokenKind.decrement => {
                var count: usize = 1;
                while (index.* < instructions.len - 1 and instructions[index.* + 1] == TokenKind.decrement) {
                    count = count + 1;
                    index.* = index.* + 1;
                }
                try generate_assembly_decrement(buffer, count);
            },
            TokenKind.output => try generate_assembly_output(buffer),
            TokenKind.input => try generate_assembly_input(buffer),
            TokenKind.jump_forward => {
                const label = index.*;
                try generate_assembly_jump_forward(buffer, label);
                index.* = index.* + 1;
                try generate_assembly_block(buffer, instructions, index);
                try generate_assembly_jump_backward(buffer, label);
            },
            TokenKind.jump_backward => return,
            TokenKind.comment => {},
        }

        index.* = index.* + 1;
    }
}

fn generate_assembly(instructions: []TokenKind) anyerror![]u8 {
    var buffer = std.ArrayList(u8).init(std.heap.page_allocator);

    try buffer.appendSlice("global _start\n");
    try buffer.appendSlice("\n");
    try buffer.appendSlice("section .text\n");
    try buffer.appendSlice("_start:\n");
    try buffer.appendSlice("    mov rax, 12                 ; system call for brk\n");
    try buffer.appendSlice("    mov rdi, 0                  ; no arguments\n");
    try buffer.appendSlice("    syscall                     ; invoke operating system to do the brk\n");
    try buffer.appendSlice("\n");
    try buffer.appendSlice("    mov [tape], rax             ; save pointer to tape\n");
    try buffer.appendSlice("    mov [pointer], rax          ; save pointer to current cell\n");
    try buffer.appendSlice("\n");
    try buffer.appendSlice("    add rax, 30000              ; move pointer to end of tape\n");
    try buffer.appendSlice("    mov rdi, rax\n");
    try buffer.appendSlice("    mov rax, 12                 ; system call for brk\n");
    try buffer.appendSlice("    syscall                     ; invoke operating system to do the brk\n");

    try buffer.appendSlice("\n");

    var index: usize = 0;
    try generate_assembly_block(&buffer, instructions, &index);

    try buffer.appendSlice("\n");

    try buffer.appendSlice("    mov rax, 12                 ; system call for brk\n");
    try buffer.appendSlice("    mov rdi, tape               ; pointer to start of tape\n");
    try buffer.appendSlice("    syscall                     ; invoke operating system to do the brk\n");
    try buffer.appendSlice("\n");
    try buffer.appendSlice("    mov rax, 60                 ; system call for exit\n");
    try buffer.appendSlice("    xor rdi, rdi                ; exit code 0\n");
    try buffer.appendSlice("    syscall                     ; invoke operating system to exit\n");
    try buffer.appendSlice("\n");
    try buffer.appendSlice("section .data\n");
    try buffer.appendSlice("tape: dq 0                      ; pointer to tape\n");
    try buffer.appendSlice("pointer: dq 0                   ; pointer to current cell\n");

    return buffer.toOwnedSlice();
}

pub fn main() !void {
    const allocator = std.heap.page_allocator;

    var args = try std.process.argsWithAllocator(allocator);
    defer args.deinit();
    _ = args.skip();
    const my_args = try parseArgs(&args);

    const program = try readFile(my_args.input);

    const instructions = try instructionsParse(program);

    const matching = try matching_brackets(instructions);
    switch (matching) {
        MatchingBracketsTag.ok => {},
        MatchingBracketsTag.not_ok_jb => |index| display_error(my_args.input, program, index, "Unmatched Jump Backward"),
        MatchingBracketsTag.not_ok_jf => |index| display_error(my_args.input, program, index, "Unmatched Jump Forward"),
    }

    const output = try generate_assembly(instructions);

    try writeFile(my_args.output, output);
}
