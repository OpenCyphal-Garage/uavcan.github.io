# UAVCAN website

[![Gitter](https://img.shields.io/badge/gitter-join%20chat-green.svg)](https://gitter.im/UAVCAN/general)

This repository contains sources of the UAVCAN website.

## Cloning the repository
`git clone https://github.com/UAVCAN/uavcan.github.io.git`
## Dependencies
- [Ruby](https://www.ruby-lang.org/en/) version 2.0.0 +  
Download the [archive](https://cache.ruby-lang.org/pub/ruby/2.3/ruby-2.3.0.tar.gz) and unpack it.  
Enter the derictory where you unpacked the archive and execute: `./configure && sudo make install` 
- [Bundler](http://bundler.io/)  
Execute from the terminal `sudo gem install bundler` 
- [Jekyll](http://jekyllrb.com/)  
Enter your local clone derictory (make sure you have Gemfile there).  
Execute from the termianl `bundle install`

## Running the website locally
Enter your local clone derictory (where the Gemfile is located).  
Execute from the terminal `bundle exec jekyll serve` and wait until the page is generated. Open your internet browser and navigate to `http://localhost:4000` to see your local website.  

## Troubleshooting 
In case you have an error like:  
***in 'autodetect': Could not find a JavaScript runtime.***  
while running `bundle exec jekyll serve`, add the following lines to the **Gemfile** in the parent folder:  
*gem 'execjs'*  
*gem 'therubyracer'*  
then open the terminal and run `bundle update && bundle install`