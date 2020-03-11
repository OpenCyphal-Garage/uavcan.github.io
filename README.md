# Legacy UAVCAN website

[![Forum](https://img.shields.io/discourse/users.svg?server=https%3A%2F%2Fforum.uavcan.org&color=1700b3)](https://forum.uavcan.org)

This repository contains sources of the legacy UAVCAN website available at <http://legacy.uavcan.org>.

How to configure Jekyll with Github Pages: <https://help.github.com/articles/using-jekyll-with-pages>.

## Dependencies

### [Ruby](https://www.ruby-lang.org/en/)

Install Ruby 2.x. If necessary, activate it using [RVM](https://rvm.io/).

### [Bundler](http://bundler.io/)

Follow the instructions on the [repository page](https://github.com/bundler/bundler) to install Bunler.

### [Jekyll](http://jekyllrb.com/)

Enter your local clone derictory (make sure you have Gemfile there). Execute from the terminal: `bundle install`.

## Running the website locally

1. Enter your local clone derictory (where the Gemfile is located).
Make sure that all git submodules are initialized by running `git submodule update --init --recursive`.
2. Execute from the terminal `bundle exec jekyll serve` and wait until the page is generated.
3. Open your internet browser and navigate to <http://localhost:4000> to see your local website.

## Troubleshooting

In case you have an error like `in 'autodetect': Could not find a JavaScript runtime.`
while running `bundle exec jekyll serve`, add the following lines to the `Gemfile` in the parent folder:

```
gem 'execjs'
gem 'therubyracer'
```

Then open the terminal and run `bundle update && bundle install`.
