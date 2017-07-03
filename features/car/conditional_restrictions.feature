@routing @car @restrictions
Feature: Car - Turn restrictions
# Handle turn restrictions as defined by http://wiki.openstreetmap.org/wiki/Relation:restriction
# Note that if u-turns are allowed, turn restrictions can lead to suprising, but correct, routes.

    Background: Use car routing
        Given the profile "car"
        Given a grid size of 200 meters
        Given the origin -9.2972,10.3811
        # coordinate in Guinée, a country that observes GMT year round

    @no_turning @conditionals
    Scenario: Car - ignores unrecognized restriction
        Given the extract extra arguments "--parse-conditional-restrictions"
                                            # time stamp for 10am on Tues, 02 May 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the node map
            """
              n
            p j e
              s
            """

        And the ways
            | nodes | oneway |
            | nj    | no     |
            | js    | no     |
            | ej    | yes    |
            | jp    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional                |
            | restriction | ej       | nj     | j        | only_right_turn @ (has_pygmies > 10 p) |

        When I route I should get
            | from | to | route    |
            | e    | s  | ej,js,js |
            | e    | n  | ej,nj,nj |
            | e    | p  | ej,jp,jp |

    @no_turning @conditionals
    Scenario: Car - Restriction would be on, but the restriction was badly tagged
        Given the extract extra arguments "--parse-conditional-restrictions"
                                            # time stamp for 10am on Tues, 02 May 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"

        Given the node map
            """
              n
           p  |
            \ |
              j
              | \
              s  m
            """

        And the ways
            | nodes |
            | nj    |
            | js    |
            | pjm   |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional             |
            | restriction | nj       | pjm    | j        | no_left_turn @ (Mo-Fr 07:00-10:30)  |
            | restriction | js       | pjm    | j        | no_right_turn @ (Mo-Fr 07:00-10:30) |

        When I route I should get
            | from | to | route      |
            | n    | m  | nj,pjm,pjm |
            | s    | m  | js,pjm,pjm |

    @no_turning @conditionals
    Scenario: Car - ignores except restriction
        Given the extract extra arguments "--parse-conditional-restrictions"
                                            # time stamp for 10am on Tues, 02 May 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the node map
            """
              n
            p j e
              s
            """

        And the ways
            | nodes | oneway |
            | nj    | no     |
            | js    | no     |
            | ej    | no     |
            | jp    | no     |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional               | except   |
            | restriction | ej       | nj     | j        | only_right_turn @ (Mo-Su 08:00-12:00) | motorcar |
            | restriction | jp       | nj     | j        | only_left_turn @ (Mo-Su 08:00-12:00)  | bus      |

        When I route I should get
            | from | to | route          | # |
            | e    | s  | ej,js,js       |   |
            | e    | n  | ej,nj,nj       | restriction does not apply to cars |
            | e    | p  | ej,jp,jp       |   |
            | p    | s  | jp,nj,nj,js,js | restriction excepting busses still applies to cars  |

    @no_turning @conditionals
    Scenario: Car - only_right_turn
        Given the extract extra arguments "--parse-conditional-restrictions"
                                            # time stamp for 10am on Tues, 02 May 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the node map
            """
              n
            p j e
              s
            """

        And the ways
            | nodes | oneway |
            | nj    | no     |
            | js    | no     |
            | ej    | yes    |
            | jp    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional               |
            | restriction | ej       | nj     | j        | only_right_turn @ (Mo-Su 07:00-14:00) |

        When I route I should get
            | from | to | route          |
            | e    | s  | ej,nj,nj,js,js |
            | e    | n  | ej,nj,nj       |
            | e    | p  | ej,nj,nj,jp,jp |

    @no_turning @conditionals
    Scenario: Car - No right turn
        Given the extract extra arguments "--parse-conditional-restrictions"
                                            # time stamp for 10am on Tues, 02 May 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the node map
            """
              n
            p j e
              s
            """

        And the ways
            | nodes | oneway |
            | nj    | no     |
            | js    | no     |
            | ej    | yes    |
            | jp    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional               |
            | restriction | ej       | nj     | j        | no_right_turn @ (Mo-Fr 07:00-13:00)   |

        When I route I should get
            | from | to | route          | # |
            | e    | s  | ej,js,js       | normal turn |
            | e    | n  | ej,js,js,nj,nj | avoids right turn |
            | e    | p  | ej,jp,jp       | normal maneuver |

    @only_turning @conditionals
    Scenario: Car - only_left_turn
        Given the extract extra arguments "--parse-conditional-restrictions"
                                            # time stamp for 10am on Tues, 02 May 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the node map
            """
              n
            p j e
              s
            """

        And the ways
            | nodes | oneway |
            | nj    | no     |
            | js    | no     |
            | ej    | yes    |
            | jp    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional              |
            | restriction | ej       | js     | j        | only_left_turn @ (Mo-Fr 07:00-16:00) |

        When I route I should get
            | from | to | route          |
            | e    | s  | ej,js,js       |
            | e    | n  | ej,js,js,nj,nj |
            | e    | p  | ej,js,js,jp,jp |

    @no_turning @conditionals
    Scenario: Car - No left turn
        Given the extract extra arguments "--parse-conditional-restrictions"
                                            # time stamp for 10am on Tues, 02 May 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the node map
            """
              n
            p j e
              s
            """

        And the ways
            | nodes | oneway |
            | nj    | no     |
            | js    | no     |
            | ej    | yes    |
            | jp    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional            |
            | restriction | ej       | js     | j        | no_left_turn @ (Mo-Su 00:00-23:59) |

        When I route I should get
            | from | to | route          |
            | e    | s  | ej,nj,nj,js,js |
            | e    | n  | ej,nj,nj       |
            | e    | p  | ej,jp,jp       |

    @no_turning @conditionals
    Scenario: Car - Conditional restriction is off
        Given the extract extra arguments "--parse-conditional-restrictions"
                                            # time stamp for 10am on Tues, 02 May 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the node map
            """
              n
            p j e
              s
            """

        And the ways
            | nodes | oneway |
            | nj    | no     |
            | js    | no     |
            | ej    | yes    |
            | jp    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional             |
            | restriction | ej       | nj     | j        | no_right_turn @ (Mo-Su 16:00-20:00) |

        When I route I should get
            | from | to | route    |
            | e    | s  | ej,js,js |
            | e    | n  | ej,nj,nj |
            | e    | p  | ej,jp,jp |

    @no_turning @conditionals
    Scenario: Car - Conditional restriction is on
        Given the extract extra arguments "--parse-conditional-restrictions"
        # 10am utc, wed
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493805600"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493805600"
        Given the node map
            """
              n
            p j e
              s
            """

        And the ways
            | nodes | oneway |
            | nj    | no     |
            | js    | no     |
            | ej    | yes    |
            | jp    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional             |
            | restriction | ej       | nj     | j        | no_right_turn @ (Mo-Fr 07:00-14:00) |

        When I route I should get
            | from | to | route          |
            | e    | s  | ej,js,js       |
            | e    | n  | ej,js,js,nj,nj |
            | e    | p  | ej,jp,jp       |

    @no_turning @conditionals
    Scenario: Car - Conditional restriction with multiple time windows
        Given the extract extra arguments "--parse-conditional-restrictions"
        # 5pm Wed 02 May, 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493744400"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493744400"

        Given the node map
            """
              n
           p  |
            \ |
              j
              | \
              s  m
            """

        And the ways
            | nodes | oneway |
            | nj    | no     |
            | js    | no     |
            | jp    | yes    |
            | mj    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional                         |
            | restriction | nj       | jp     | j        | no_right_turn @ (Mo-Fr 07:00-11:00,16:00-18:30) |

        When I route I should get
            | from | to | route          |
            | n    | p  | nj,js,js,jp,jp |
            | m    | p  | mj,jp,jp       |

    @no_turning @conditionals
    Scenario: Car - only_right_turn
        Given the extract extra arguments "--parse-conditional-restrictions"
                                            # time stamp for 10am on Tues, 02 May 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the node map
            """
              n
            p j e
              s
            """

        And the ways
            | nodes | oneway |
            | nj    | no     |
            | js    | no     |
            | ej    | yes    |
            | jp    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional               |
            | restriction | ej       | nj     | j        | only_right_turn @ (Mo-Su 07:00-14:00) |

        When I route I should get
            | from | to | route          |
            | e    | s  | ej,nj,nj,js,js |
            | e    | n  | ej,nj,nj       |
            | e    | p  | ej,nj,nj,jp,jp |

    @no_turning @conditionals
    Scenario: Car - No right turn
        Given the extract extra arguments "--parse-conditional-restrictions"
                                            # time stamp for 10am on Tues, 02 May 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the node map
            """
              n
            p j e
              s
            """

        And the ways
            | nodes | oneway |
            | nj    | no     |
            | js    | no     |
            | ej    | yes    |
            | jp    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional               |
            | restriction | ej       | nj     | j        | no_right_turn @ (Mo-Fr 07:00-13:00)   |

        When I route I should get
            | from | to | route          | # |
            | e    | s  | ej,js,js       | normal turn |
            | e    | n  | ej,js,js,nj,nj | avoids right turn |
            | e    | p  | ej,jp,jp       | normal maneuver |

    @only_turning @conditionals
    Scenario: Car - only_left_turn
        Given the extract extra arguments "--parse-conditional-restrictions"
                                            # time stamp for 10am on Tues, 02 May 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the node map
            """
              n
            p j e
              s
            """

        And the ways
            | nodes | oneway |
            | nj    | no     |
            | js    | no     |
            | ej    | yes    |
            | jp    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional              |
            | restriction | ej       | js     | j        | only_left_turn @ (Mo-Fr 07:00-16:00) |

        When I route I should get
            | from | to | route          |
            | e    | s  | ej,js,js       |
            | e    | n  | ej,js,js,nj,nj |
            | e    | p  | ej,js,js,jp,jp |

    @no_turning @conditionals
    Scenario: Car - No left turn
        Given the extract extra arguments "--parse-conditional-restrictions"
                                            # time stamp for 10am on Tues, 02 May 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the node map
            """
              n
            p j e
              s
            """

        And the ways
            | nodes | oneway |
            | nj    | no     |
            | js    | no     |
            | ej    | yes    |
            | jp    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional            |
            | restriction | ej       | js     | j        | no_left_turn @ (Mo-Su 00:00-23:59) |

        When I route I should get
            | from | to | route          |
            | e    | s  | ej,nj,nj,js,js |
            | e    | n  | ej,nj,nj       |
            | e    | p  | ej,jp,jp       |

    @no_turning @conditionals
    Scenario: Car - Conditional restriction is off
        Given the extract extra arguments "--parse-conditional-restrictions"
                                            # time stamp for 10am on Tues, 02 May 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493719200"
        Given the node map
            """
              n
            p j e
              s
            """

        And the ways
            | nodes | oneway |
            | nj    | no     |
            | js    | no     |
            | ej    | yes    |
            | jp    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional             |
            | restriction | ej       | nj     | j        | no_right_turn @ (Mo-Su 16:00-20:00) |

        When I route I should get
            | from | to | route    |
            | e    | s  | ej,js,js |
            | e    | n  | ej,nj,nj |
            | e    | p  | ej,jp,jp |

    @no_turning @conditionals
    Scenario: Car - Conditional restriction is on
        Given the extract extra arguments "--parse-conditional-restrictions"
        # 10am utc, wed
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493805600"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493805600"
        Given the node map
            """
              n
            p j e
              s
            """

        And the ways
            | nodes | oneway |
            | nj    | no     |
            | js    | no     |
            | ej    | yes    |
            | jp    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional             |
            | restriction | ej       | nj     | j        | no_right_turn @ (Mo-Fr 07:00-14:00) |

        When I route I should get
            | from | to | route          |
            | e    | s  | ej,js,js       |
            | e    | n  | ej,js,js,nj,nj |
            | e    | p  | ej,jp,jp       |

    @no_turning @conditionals
    Scenario: Car - Conditional restriction with multiple time windows
        Given the extract extra arguments "--parse-conditional-restrictions"
        # 5pm Wed 02 May, 2017 GMT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493744400"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/guinea.geojson --parse-conditionals-from-now=1493744400"

        Given the node map
            """
              n
           p  |
            \ |
              j
              | \
              s  m
            """

        And the ways
            | nodes | oneway |
            | nj    | no     |
            | js    | no     |
            | jp    | yes    |
            | mj    | yes    |

        And the relations
            | type        | way:from | way:to | node:via | restriction:conditional                         |
            | restriction | nj       | jp     | j        | no_right_turn @ (Mo-Fr 07:00-11:00,16:00-18:30) |

        When I route I should get
            | from | to | route          |
            | n    | p  | nj,js,js,jp,jp |
            | m    | p  | mj,jp,jp       |

    # https://www.openstreetmap.org/#map=18/38.91099/-77.00888
    @no_turning @conditionals
    Scenario: Car - DC North capitol situation, two on one off
        Given the extract extra arguments "--parse-conditional-restrictions=1"
        # 9pm Wed 02 May, 2017 UTC, 5pm EDT
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/dc.geojson --parse-conditionals-from-now=1493845200"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/dc.geojson --parse-conditionals-from-now=1493845200"

        #    """
        #      a h
        #   d
        #      b g
        #           e
        #      c f
        #    """
        Given the node locations
            | node | lat     | lon      |
            | a    | 38.9113 | -77.0091 |
            | b    | 38.9108 | -77.0091 |
            | c    | 38.9104 | -77.0091 |
            | d    | 38.9110 | -77.0096 |
            | e    | 38.9106 | -77.0086 |
            | f    | 38.9105 | -77.0090 |
            | g    | 38.9108 | -77.0090 |
            | h    | 38.9113 | -77.0090 |

        And the ways
            | nodes | oneway | name       |
            | ab    | yes    | cap south  |
            | bc    | yes    | cap south  |
            | fg    | yes    | cap north  |
            | gh    | yes    | cap north  |
            | db    | no     | florida nw |
            | bg    | no     | florida    |
            | ge    | no     | florida ne |

        And the relations
            | type        | way:from  | way:to  | node:via | restriction:conditional                        |
            | restriction | ab        | bg      | b        | no_left_turn @ (Mo-Fr 07:00-09:30,16:00-18:30) |
            | restriction | fg        | bg      | g        | no_left_turn @ (Mo-Fr 06:00-10:00)             |
            | restriction | bg        | bc      | b        | no_left_turn @ (Mo-Fr 07:00-09:30,16:00-18:30) |

        When I route I should get
            | from | to | route                                      | turns                                       |
            | a    | e  | cap south,florida nw,florida nw,florida ne | depart,turn right,continue uturn,arrive     |
            | f    | d  | cap north,florida,florida nw               | depart,turn left,arrive                     |
            | e    | c  | florida ne,florida nw,cap south,cap south  | depart,continue uturn,turn right,arrive     |

    @no_turning @conditionals
    Scenario: Car - DC North capitol situation, one on two off
        Given the extract extra arguments "--parse-conditional-restrictions=1"
        # 10:30am utc, wed, 6:30am est
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/dc.geojson --parse-conditionals-from-now=1493807400"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/dc.geojson --parse-conditionals-from-now=1493807400"

        #    """
        #      a h
        #   d
        #      b g
        #           e
        #      c f
        #    """
        Given the node locations
            | node | lat     | lon      |
            | a    | 38.9113 | -77.0091 |
            | b    | 38.9108 | -77.0091 |
            | c    | 38.9104 | -77.0091 |
            | d    | 38.9110 | -77.0096 |
            | e    | 38.9106 | -77.0086 |
            | f    | 38.9105 | -77.0090 |
            | g    | 38.9108 | -77.0090 |
            | h    | 38.9113 | -77.0090 |

        And the ways
            | nodes | oneway | name       |
            | ab    | yes    | cap south  |
            | bc    | yes    | cap south  |
            | fg    | yes    | cap north  |
            | gh    | yes    | cap north  |
            | db    | no     | florida nw |
            | bg    | no     | florida    |
            | ge    | no     | florida ne |

        And the relations
            | type        | way:from  | way:to  | node:via | restriction:conditional                        |
            | restriction | ab        | bg      | b        | no_left_turn @ (Mo-Fr 07:00-09:30,16:00-18:30) |
            | restriction | fg        | bg      | g        | no_left_turn @ (Mo-Fr 06:00-10:00)             |
            | restriction | bg        | bc      | b        | no_left_turn @ (Mo-Fr 07:00-09:30,16:00-18:30) |

        When I route I should get
            | from | to | route                                      | turns                                         |
            | a    | e  | cap south,florida,florida ne               | depart,turn left,arrive                       |
            | f    | d  | cap north,florida ne,florida ne,florida nw | depart,turn sharp right,continue uturn,arrive |
            | e    | c  | florida ne,cap south,cap south             | depart,turn left,arrive                       |

    @only_turning @conditionals
    Scenario: Car - Restriction is always off when point not found in timezone files
        # same test as the following one, but given a different time zone file
        Given the extract extra arguments "--parse-conditional-restrictions"
        # 9am UTC, 10am BST
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/dc.geojson --parse-conditionals-from-now=1493802000"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/dc.geojson --parse-conditionals-from-now=1493802000"

        #    """
        #     a
        #          e
        #      b
        #   d
        #       c
        #    """
        Given the node locations
            | node | lat     | lon     |
            | a    | 51.5250 | -0.1166 |
            | b    | 51.5243 | -0.1159 |
            | c    | 51.5238 | -0.1152 |
            | d    | 51.5241 | -0.1167 |
            | e    | 51.5247 | -0.1153 |

        And the ways
            | nodes | name  |
            | ab    | albic |
            | bc    | albic |
            | db    | dobe |
            | be    | dobe |

        And the relations
            | type        | way:from  | way:to  | node:via | restriction:conditional               |
            | restriction | ab        | be      | b        | only_left_turn @ (Mo-Fr 07:00-11:00)  |

        When I route I should get
            | from | to | route           | turns                   |
            | a    | c  | albic,albic     | depart,arrive           |
            | a    | e  | albic,dobe,dobe | depart,turn left,arrive |

    @only_turning @conditionals
    Scenario: Car - Somewhere in london, the UK, GMT timezone
        Given the extract extra arguments "--parse-conditional-restrictions"
        # 9am UTC, 10am BST
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/london.geojson --parse-conditionals-from-now=1493802000"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/london.geojson --parse-conditionals-from-now=1493802000"

        #    """
        #     a
        #          e
        #      b
        #   d
        #       c
        #    """
        Given the node locations
            | node | lat     | lon     |
            | a    | 51.5250 | -0.1166 |
            | b    | 51.5243 | -0.1159 |
            | c    | 51.5238 | -0.1152 |
            | d    | 51.5241 | -0.1167 |
            | e    | 51.5247 | -0.1153 |

        And the ways
            | nodes | name  |
            | ab    | albic |
            | bc    | albic |
            | db    | dobe |
            | be    | dobe |

        And the relations
            | type        | way:from  | way:to  | node:via | restriction:conditional               |
            | restriction | ab        | be      | b        | only_left_turn @ (Mo-Fr 07:00-11:00)  |

        When I route I should get
            | from | to | route                       | turns                                            |
            | a    | c  | albic,dobe,dobe,albic,albic | depart,turn left,continue uturn,turn left,arrive |
            | a    | e  | albic,dobe,dobe             | depart,turn left,arrive                          |

    @only_turning @conditionals
    Scenario: Car - Somewhere in London, the UK, GMT timezone
        Given the extract extra arguments "--parse-conditional-restrictions=1"
        # 9am UTC, 10am BST
        Given the contract extra arguments "--time-zone-file=test/data/tz/{timezone_names}/london.geojson --parse-conditionals-from-now=1493802000"
        Given the customize extra arguments "--time-zone-file=test/data/tz/{timezone_names}/london.geojson --parse-conditionals-from-now=1493802000"

        #    """
        #     a
        #          e
        #      b
        #   d
        #       c
        #    """
        Given the node locations
            | node | lat     | lon     |
            | a    | 51.5250 | -0.1166 |
            | b    | 51.5243 | -0.1159 |
            | c    | 51.5238 | -0.1152 |
            | d    | 51.5241 | -0.1167 |
            | e    | 51.5247 | -0.1153 |

        And the ways
            | nodes | name  |
            | ab    | albic |
            | bc    | albic |
            | db    | dobe |
            | be    | dobe |

        And the relations
            | type        | way:from  | way:to  | node:via | restriction:conditional               |
            | restriction | ab        | be      | b        | only_left_turn @ (Mo-Fr 07:00-11:00)  |

        When I route I should get
            | from | to | route                       | turns                                            |
            | a    | c  | albic,dobe,dobe,albic,albic | depart,turn left,continue uturn,turn left,arrive |
            | a    | e  | albic,dobe,dobe             | depart,turn left,arrive                          |
