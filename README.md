# Distmark

Originally conceived as _dist_ributed [post_mark_](http://www.filesystems.org/docs/auto-pilot/Postmark.html),
this trusty little benchmark has been helping me assess performance of storage
systems for half a decade now.

If you're interested in the gory details, take a look at my
[How to run relevant benchmarks](https://blog.svedr.in/posts/how-to-run-relevant-benchmarks/)
blog post that explains why I wrote distmark. Here's the gist:

> In the past, I've had some success running multiple instances of Postmark
> in parallel. Postmark simulates a busy mail server by
> creating/writing to/deleting lots of small files, generating lots of random
> IO in the process. Unfortunately, Postmark simply does as much as possible,
> firing up the %util column in iostat to 100%.
>
> By itself, that wouldn't be too bad — isn't that actually our goal, knowing
> what the system is capable of?
>
> The problem is that when the bus nears saturation, the system responds in a
> completely different way than normally. Most notably, the latency skyrockets
> by multiple orders of magnitude. A system that has a latency of as low as
> 0.06ms may easily crawl to a halt when maxed out, then reporting latencies
> of 50ms or more. That's an 800-fold increase! Needless to say, bad things™
> will ensue.
>
> So, we'll have to add another bullet point to the list:
>
> * Never ever max out the system during a benchmark.
>
> In order to get a feeling for the maximum load a system will be able to take,
> see what the latency is when running at about 30%. Then slowly increase the
> workload, and see how far you can go before the latency gets unacceptable.

# Usage

1.  Mount yerself a filesystem of some 50GB that can take some load somewhere.
    My personal favorite is XFS, tuned as such:

    ```
    mkfs -t xfs -b size=4096 -s size=512 -d su=256k -d sw=2 -l su=256k /dev/sas1/perftest
    ```

    Note that for these options to be beneficial, the underlying RAID must
    use precisely four data disks and a stripe width of 256KiB. I've found that
    if you're not absolutely positive that this is the hardware setup you're
    running on, omitting _any_ tuning options and just using the defaults yields
    better results. You have been warned.

2.  Run distmark on that FS with a reasonable number of processes and IOPS, such as:

    ```
    ./distmark /mnt 16 1000
    ```

3.  Run `iostat -xdm 10 /dev/sd?` in another shell (or tmux pane) to see what
    impact you're having. Specifically, focus on the `w/s`, `w_await` and `%util`
    columns. You'll want your `w_await` to be around 1ms and your `%util` around
    30%. If you meet those conditions, the `w/s` will tell you what load your
    system is capable of _under normal circumstances_. You can run that load
    in production, 24/7, and still sleep like a baby.
