# remoteStorage-fuse - Mount your remoteStorage as filesystem in userspace

This is an implementation of the remoteStorage protocol that can be used to 
access data stored on a remoteStorage compatible storage server via the regular 
filesystem.

This is still work in progress, the following things are expected to work:
* Mount a remoteStorage, given a base_url and a bearer token granting 
  root-access
* List directories
* Read files
* Write files (WARNING: destroys MIME types. All MIME types for files edited 
  via the fuse plugin will be set to 
  `application/octet-stream; charset=binary`)
* Delete files

The following things do not work yet, but are planned:
* MIME types
* In-memory caching of files and / or the directory tree
* Webfinger discovery (so you don't have to know your base_url)
* Authorization flow (so you don't have to copy tokens from somewhere)

The directory handling is a lot different from regular filesystems:
* mkdir() will always succeed, but not actually create empty directories
* write()ing to a file will implicitly create the file and any parent 
  directories.
* rmdir() will also always succeed.

Usage
-----

1) Install dependencies:

You need libcurl and libfuse to build remoteStorage-fuse. On Debian based 
systems, this should do the trick:

    $ sudo apt-get install build-essential libfuse-dev libcurl4-openssl-dev

On Fedora >= 22:

    $ sudo dnf -y install fuse-devel libcurl-devel pkgconfig gcc
    
On CentOS:

    $ sudo yum -y install fuse-devel libcurl-devel pkgconfig gcc

2) Build

Simply run

  make

and you should be good to go. There are a few warnings, which you can ignore 
for now (I promise to get rid of them before any stable release). If you see 
any errors, please report them as a github issue.

3) Find out your base URL

To actually mount your storage, you need to know two things: your storage's 
base URL and a bearer token that grants root-access to your storage.
You can find out the base URL by doing a manual webfinger discovery:

    $ curl https://<your-provider>/.well-known/webfinger?resource=acct:<you>@<your-provider>

and taking the `href` of the `remotestorage` link from the result. For example 
if you have a 5apps account and your name is `fkooman`, you would do:

    $ curl https://5apps.com/.well-known/webfinger?resource=acct:fkooman@5apps.com

and get the result:

    {
        "links": [
            {
                "href": "https://storage.5apps.com/fkooman",
                "properties": {
                    "http://remotestorage.io/spec/version": "draft-dejong-remotestorage-02",
                    "http://tools.ietf.org/html/rfc2616#section-14.16": false,
                    "http://tools.ietf.org/html/rfc6749#section-4.2": "https://5apps.com/rs/oauth/fkooman",
                    "http://tools.ietf.org/html/rfc6750#section-2.3": false
                },
                "rel": "remotestorage"
            }
        ]
    }

In this case the base URL would be `https://storage.5apps.com/fkooman`.

4) Get a bearer token

Getting a bearer token is easy as well. Just visit the remoteStorage browser at:

    https://remotestorage-browser.5apps.com/

connect your storage there, then open the JavaScript console and type:

    remoteStorage.remote.token

That will display your token in the console.

5) Mount the storage

Now that you know your base URL and bearer token, you can mount your storage:
(just replace BASE_URL and TOKEN with the values you figured out above)

    $ sudo ./rs-mount -o base_url=BASE_URL,token=TOKEN /path/to/mount/point

For example:

    $ sudo ./rs-mount -o base_url=https://storage.5apps.com/fkooman,token=8bb19ded9e0ed986c6a92494f7c8cf0d /mnt

Now you should be able to see and browse your files:

    [fkooman@noname rs-fuse]$ sudo ls /mnt
    @context  foo  items  rss  sockethub
    [fkooman@noname rs-fuse]$ 
