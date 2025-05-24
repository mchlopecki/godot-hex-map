function strip_common_indent(lines)
    local min_spaces = 10000

    -- Find the minimum number of leading spaces
    for _, str in ipairs(lines) do
        local leading_spaces = string.match(str, "^%s*")
        if leading_spaces then
            min_spaces = math.min(min_spaces, #leading_spaces)
        end
    end

    -- Strip the leading spaces and return the modified strings
    local out = {}
    for _, str in ipairs(lines) do
        local stripped = string.sub(str, min_spaces + 1)
        table.insert(out, stripped)
    end

    return min_spaces, out
end

-- test cases for strip_common_indent
local strip_common_test_cases = {
    { { "xxx", "xxx" },          0, { "xxx", "xxx" } },
    { { "   xxx", "   xxx" },    3, { "xxx", "xxx" } },
    { { "   xxx", "xxx" },       0, { "   xxx", "xxx" } },
    { { "xxx", "   xxx" },       0, { "xxx", "   xxx" } },
    { { "   xxx", "      xxx" }, 3, { "xxx", "   xxx" } },
}
for _, case in ipairs(strip_common_test_cases) do
    local indent, result = strip_common_indent(case[1])
    assert(vim.deep_equal({ indent, result }, { case[2], case[3] }),
        "strip_common_indent(" ..
        vim.inspect(case[1]) ..
        ") failed test; expected " ..
        vim.inspect(result) ..
        " to equal " ..
        vim.inspect(case[2]))
end

-- class for loading and outputting cell maps
local CellMap = {}
function CellMap:new(o)
    o = o or { rows = {} }
    setmetatable(o, self)
    self.__index = self
    return o
end

function CellMap:insert_row(line)
    -- skip dividing lines: ^----v----^
    if not string.match(line, "[^v^%-]") then
        return
    end

    if self.indent == nil then
        self.indent = string.match(line, "^%s") ~= nil
    end

    -- split the cell labels
    local row = {}
    for part in vim.gsplit(line, "|") do
        table.insert(row, vim.trim(part))
    end

    -- remove the first element; was before '|'
    table.remove(row, #row)
    table.remove(row, 1)

    row["__indent"] = self.indent
    self.indent = not self.indent
    table.insert(self.rows, row)
end

-- test cases for CellMap:insert_row
local insert_row_test_cases = {
    { { "| a | b |" }, { { "a", "b", __indent = false } } },
    { { "|a|b|" },     { { "a", "b", __indent = false } } },
    { { "||b|" },      { { "", "b", __indent = false } } },
    { { " |a|b|" },    { { "a", "b", __indent = true } } },
    { { "^---v---^" }, {} },
    { { "v---^---v" }, {} },
    {
        {
            "  | a | b |",
            "|c|d|"
        },
        {
            { "a", "b", __indent = true },
            { "c", "d", __indent = false },
        }
    },
    {
        { "^---v---^---v---^---v",
            "    | a     |   b   |",
            "v---^---v---^---v---^",
            "|   x   |   y   |",
            "^---v---^---v---^---v",
        },
        {
            { "a", "b", __indent = true },
            { "x", "y", __indent = false },
        },
    },
}
for _, case in ipairs(insert_row_test_cases) do
    local map = CellMap:new()
    for _, line in ipairs(case[1]) do
        map:insert_row(line)
    end
    local expect = case[2]
    assert(vim.deep_equal(map.rows, expect),
        "CellMap:insert_row(" .. vim.inspect(case[1]) .. ") failed test; " ..
        "expected " ..
        vim.inspect(map.rows) ..
        " to equal " ..
        vim.inspect(expect))
end

function make_separator_generator(width)
    local left_width = math.floor((width - 1) / 2)
    local right_width = width - 1 - left_width
    local chunks = {
        up = {
            string.rep("-", left_width) .. "v",
            string.rep("-", right_width) .. "^",
        },
        down = {
            string.rep("-", left_width) .. "^",
            string.rep("-", right_width) .. "v",
        },
    }
    local last_segments = 0

    return function(columns, indent)
        -- how many segments to we need to draw?  A segment is "---^" or "---v"
        local row_segments = columns * 2 + (indent and 1 or 0)
        local segments = math.max(row_segments, last_segments)

        local out = indent and "^" or "v"
        local chunk = indent and chunks.down or chunks.up
        for index = 1, segments do
            out = out .. chunk[(index % 2) + 1]
        end

        last_segments = row_segments
        return out
    end
end

function make_cell_generator(width)
    local left_width = math.floor((width - 1) / 2)
    local right_width = width - left_width
    local label_width = width - 2

    return function(cells)
        local out = cells.__indent and string.rep(" ", right_width) or ""

        for key, value in pairs(cells) do
            if type(key) == "number" then
                out = out .. string.format("| %-" .. label_width .. "s ", value)
            end
        end
        return out .. "|"
    end
end

function CellMap:format()
    if #self.rows == 0 then
        return {}
    end

    -- calculate the cell width; minimum width is 5 characters, max is longest
    -- label with space padding on both sides (label_max + 2)
    local label_max = 0
    for _, row in ipairs(self.rows) do
        for key, value in pairs(row) do
            if type(key) == "number" then
                if #value > label_max then
                    label_max = #value
                end
            end
        end
    end

    local col_width = math.max(label_max + 2, 5)
    local label_width = col_width - 2

    -- function for generating cell bodies
    local cell_generator = make_cell_generator(col_width)
    local separater = make_separator_generator(col_width)

    local out = {}
    local last_indent = nil
    for _, row in ipairs(self.rows) do
        table.insert(out, separater(#row, row.__indent))
        table.insert(out, cell_generator(row))
        last_indent = row.__indent
    end
    table.insert(out, separater(0, not last_indent))
    return out
end

local format_test_cases = {
    {
        {},
        {},
    },
    {
        { { "a", __indent = false }, },
        {
            "v--^--v",
            "| a   |",
            "^--v--^", },
    },
    {
        { { "a", __indent = true }, },
        {
            "^--v--^--v",
            "   | a   |",
            "v--^--v--^", },
        {
            "v--^--v--^",
            "   | a   |",
            "v--^--v--^",
        },
    },
    {
        { { "longer", __indent = false }, },
        {
            "v----^---v",
            "| longer |",
            "^----v---^", },
    },
    {
        { { "longer", __indent = true }, },
        {
            "^----v---^----v",
            "     | longer |",
            "v----^---v----^", },
    },
    {
        { { "a", "b", "c", "d", "e", __indent = false }, },
        {
            "v--^--v--^--v--^--v--^--v--^--v",
            "| a   | b   | c   | d   | e   |",
            "^--v--^--v--^--v--^--v--^--v--^" },
    },
    {
        {
            { "a", "b", "c", __indent = false },
            { "x", "y", "z", __indent = true },
        },
        {
            "v--^--v--^--v--^--v",
            "| a   | b   | c   |",
            "^--v--^--v--^--v--^--v",
            "   | x   | y   | z   |",
            "v--^--v--^--v--^--v--^",
        },
    },
    {
        {
            { "x", "y", __indent = true },
            { "a", "b", __indent = false },
        },
        {
            "^--v--^--v--^--v",
            "   | x   | y   |",
            "v--^--v--^--v--^",
            "| a   | b   |",
            "^--v--^--v--^",
        },
    },
    {
        {
            { "x", "y",             "z", __indent = true },
            { "a", __indent = false },
        },
        {
            "^--v--^--v--^--v--^--v",
            "   | x   | y   | z   |",
            "v--^--v--^--v--^--v--^",
            "| a   |",
            "^--v--^",
        },
    },
    {
        {
            { "a", __indent = false },
            { "x", "y",             "z", __indent = true },
        },
        {
            "v--^--v",
            "| a   |",
            "^--v--^--v--^--v--^--v",
            "   | x   | y   | z   |",
            "v--^--v--^--v--^--v--^",
        },
    },
    {
        {
            { "a", "b", "c", __indent = false },
            { "x", "y", "z", __indent = true },
            { "1", "2", "3", __indent = false },
        },
        {
            "v--^--v--^--v--^--v",
            "| a   | b   | c   |",
            "^--v--^--v--^--v--^--v",
            "   | x   | y   | z   |",
            "v--^--v--^--v--^--v--^",
            "| 1   | 2   | 3   |",
            "^--v--^--v--^--v--^",
        },
    },
}
for _, case in ipairs(format_test_cases) do
    local map = CellMap:new({ rows = case[1] })
    local expect = case[2]
    local result = map:format()
    assert(vim.deep_equal(result, expect),
        "CellMap:format() for rows " .. vim.inspect(case[1]) .. " failed; " ..
        "expected " ..
        vim.inspect(result) ..
        " to equal " ..
        vim.inspect(expect))
end


function format_cells(lines)
    local cells = CellMap:new()
    for _, line in ipairs(lines) do
        cells:insert_row(line)
    end
    return cells:format()
end

vim.api.nvim_create_user_command("FormatHex",
    function(range)
        local lines = vim.fn.getline(range.line1, range.line2)
        local indent, lines = strip_common_indent(lines)
        indent = string.rep(" ", indent)
        local formatted = format_cells(lines)
        for i, line in ipairs(formatted) do
            formatted[i] = indent .. line
        end

        vim.api.nvim_buf_set_lines(
            vim.api.nvim_get_current_buf(),
            range.line1 - 1,
            range.line2,
            false,
            formatted)
    end,
    { range = true })
