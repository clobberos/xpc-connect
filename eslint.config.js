const js = require("@eslint/js");
const prettier = require("eslint-config-prettier");
const prettierPlugin = require("eslint-plugin-prettier");

module.exports = [
  js.configs.recommended,
  {
    files: ["**/*.js"],
    languageOptions: {
      ecmaVersion: 2020,
      sourceType: "commonjs",
      globals: {
        // Node.js globals
        __dirname: "readonly",
        __filename: "readonly",
        Buffer: "readonly",
        console: "readonly",
        exports: "writable",
        global: "readonly",
        module: "readonly",
        process: "readonly",
        require: "readonly",
        setImmediate: "readonly",
        clearImmediate: "readonly",
        setInterval: "readonly",
        clearInterval: "readonly",
        setTimeout: "readonly",
        clearTimeout: "readonly",
        // Jest globals
        afterAll: "readonly",
        afterEach: "readonly",
        beforeAll: "readonly",
        beforeEach: "readonly",
        describe: "readonly",
        expect: "readonly",
        it: "readonly",
        jest: "readonly",
        test: "readonly",
      },
    },
    plugins: {
      prettier: prettierPlugin,
    },
    rules: {
      "max-len": 0,
      "comma-dangle": 0,
      "no-console": 0,
      "padded-blocks": 0,
      indent: [
        2,
        2,
        {
          SwitchCase: 1,
        },
      ],
      "spaced-comment": 1,
      quotes: [
        "error",
        "single",
        {
          allowTemplateLiterals: true,
        },
      ],
      "import/prefer-default-export": "off",
      "arrow-parens": 0,
      "consistent-return": 0,
      "no-useless-escape": 0,
      "no-underscore-dangle": 0,
      "no-undef": "error",
      "prettier/prettier": "error",
    },
  },
  prettier,
];
