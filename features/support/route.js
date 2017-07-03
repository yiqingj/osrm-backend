'use strict';

const Timeout = require('node-timeout');
const request = require('request');
const ensureDecimal = require('../lib/utils').ensureDecimal;

module.exports = function () {
    this.requestPath = (service, params, callback) => {
        var uri;
        if (service == 'timestamp') {
            uri = [this.HOST, service].join('/');
        } else {
            uri = [this.HOST, service, 'v1', this.profile].join('/');
        }

        return this.sendRequest(uri, params, callback);
    };

    this.requestUrl = (path, callback) => {
        var uri = this.query = [this.HOST, path].join('/'),
            limit = Timeout(this.TIMEOUT, { err: { statusCode: 408 } });

        function runRequest (cb) {
            request(uri, cb);
        }

        runRequest(limit((err, res, body) => {
            if (err) {
                if (err.statusCode === 408) return callback(this.RoutedError('*** osrm-routed did not respond'));
                else if (err.code === 'ECONNREFUSED')
                    return callback(this.RoutedError('*** osrm-routed is not running'));
            } else
                return callback(err, res, body);
        }));
    };

    // Overwrites the default values in defaults
    // e.g. [[a, 1], [b, 2]], [[a, 5], [d, 10]] => [[a, 5], [b, 2], [d, 10]]
    this.overwriteParams = (defaults, other) => {
        var otherMap = {};
        for (var key in other) otherMap[key] = other[key];
        return Object.assign({}, defaults, otherMap);
    };

    var encodeWaypoints = (waypoints) => {
        return waypoints.map(w => [w.lon, w.lat].map(ensureDecimal).join(','));
    };

    this.requestRoute = (waypoints, bearings, approaches, userParams, callback) => {
        if (bearings.length && bearings.length !== waypoints.length) throw new Error('*** number of bearings does not equal the number of waypoints');
        if (approaches.length && approaches.length !== waypoints.length) throw new Error('*** number of approaches does not equal the number of waypoints');

        var defaults = {
                output: 'json',
                steps: 'true',
                alternatives: 'false'
            },
            params = this.overwriteParams(defaults, userParams),
            encodedWaypoints = encodeWaypoints(waypoints);

        params.coordinates = encodedWaypoints;

        if (bearings.length) {
            params.bearings = bearings.map(b => {
                var bs = b.split(',');
                if (bs.length === 2) return b;
                else return b += ',10';
            }).join(';');
        }

        if (approaches.length) {
            params.approaches = approaches.join(';');
        }
        return this.requestPath('route', params, callback);
    };

    this.requestNearest = (node, userParams, callback) => {
        var defaults = {
                output: 'json'
            },
            params = this.overwriteParams(defaults, userParams);
        params.coordinates = [[node.lon, node.lat].join(',')];

        return this.requestPath('nearest', params, callback);
    };

    this.requestTable = (waypoints, userParams, callback) => {
        var defaults = {
                output: 'json'
            },
            params = this.overwriteParams(defaults, userParams);

        params.coordinates = waypoints.map(w => [w.coord.lon, w.coord.lat].join(','));
        var srcs = waypoints.map((w, i) => [w.type, i]).filter(w => w[0] === 'src').map(w => w[1]),
            dsts = waypoints.map((w, i) => [w.type, i]).filter(w => w[0] === 'dst').map(w => w[1]);
        if (srcs.length) params.sources = srcs.join(';');
        if (dsts.length) params.destinations = dsts.join(';');

        return this.requestPath('table', params, callback);
    };

    this.requestTrip = (waypoints, userParams, callback) => {
        var defaults = {
                output: 'json'
            },
            params = this.overwriteParams(defaults, userParams);

        params.coordinates = encodeWaypoints(waypoints);

        return this.requestPath('trip', params, callback);
    };

    this.requestMatching = (waypoints, timestamps, userParams, callback) => {
        var defaults = {
                output: 'json'
            },
            params = this.overwriteParams(defaults, userParams);

        params.coordinates = encodeWaypoints(waypoints);

        if (timestamps.length) {
            params.timestamps = timestamps.join(';');
        }

        return this.requestPath('match', params, callback);
    };

    this.extractInstructionList = (instructions, keyFinder) => {
        if (instructions) {
            return instructions.legs.reduce((m, v) => m.concat(v.steps), [])
                .map(keyFinder)
                .join(',');
        }
    };

    this.summary = (instructions) => {
        if (instructions) {
            return instructions.legs.map(l => l.summary).join(';');
        }
    };

    this.wayList = (instructions) => {
        return this.extractInstructionList(instructions, s => s.name);
    };

    this.refList = (instructions) => {
        return this.extractInstructionList(instructions, s => s.ref || '');
    };

    this.pronunciationList = (instructions) => {
        return this.extractInstructionList(instructions, s => s.pronunciation || '');
    };

    this.destinationsList = (instructions) => {
        return this.extractInstructionList(instructions, s => s.destinations || '');
    };

    this.reverseBearing = (bearing) => {
        if (bearing >= 180)
            return bearing - 180.;
        return bearing + 180;
    };

    this.bearingList = (instructions) => {
        return this.extractInstructionList(instructions, s => ('in' in s.intersections[0] ? this.reverseBearing(s.intersections[0].bearings[s.intersections[0].in]) : 0)
                                                              + '->' +
                                                              ('out' in s.intersections[0] ? s.intersections[0].bearings[s.intersections[0].out] : 0));
    };

    this.approachList = (instructions) => {
        return this.extractInstructionList(instructions, s => s.approaches || '');
    };

    this.annotationList = (instructions) => {
        if (!('annotation' in instructions.legs[0]))
            return '';

        var merged = {};
        instructions.legs.map(l => {
            Object.keys(l.annotation).forEach(a => {
                if (!merged[a]) merged[a] = [];
                merged[a].push(l.annotation[a].join(':'));
            });
        });
        Object.keys(merged).map(a => {
            merged[a] = merged[a].join(',');
        });
        return merged;
    };

    this.alternativesList = (instructions) => {
        // alternatives_count come from tracepoints list
        return instructions.tracepoints.map(t => t.alternatives_count.toString()).join(',');
    };

    this.lanesList = (instructions) => {
        return this.extractInstructionList(instructions, instruction => {
            if( 'lanes' in instruction.intersections[0] )
            {
                return instruction.intersections[0].lanes.map( p => { return (p.indications).join(';') + ':' + p.valid; } ).join(' ');
            } else
            {
                return '';
            }});
    };

    this.turnList = (instructions) => {
        return instructions.legs.reduce((m, v) => m.concat(v.steps), [])
            .map(v => {
                switch (v.maneuver.type) {
                case 'depart':
                case 'arrive':
                    return v.maneuver.type;
                case 'on ramp':
                case 'off ramp':
                    return v.maneuver.type + ' ' + v.maneuver.modifier;
                case 'roundabout':
                    return 'roundabout-exit-' + v.maneuver.exit;
                case 'rotary':
                    if( 'rotary_name' in v )
                        return v.rotary_name + '-exit-' + v.maneuver.exit;
                    else
                        return 'rotary-exit-' + v.maneuver.exit;
                case 'roundabout turn':
                    return v.maneuver.type + ' ' + v.maneuver.modifier + ' exit-' + v.maneuver.exit;
                // FIXME this is a little bit over-simplistic for merge/fork instructions
                default:
                    return v.maneuver.type + ' ' + v.maneuver.modifier;
                }
            })
            .join(',');
    };

    this.locations = (instructions) => {
        return instructions.legs.reduce((m, v) => m.concat(v.steps), [])
            .map(v => {
                return this.findNodeByLocation(v.maneuver.location);
            })
            .join(',');
    };

    this.intersectionList = (instructions) => {
        return instructions.legs.reduce((m, v) => m.concat(v.steps), [])
            .map( v => {
                return v.intersections
                    .map( intersection => {
                        var string = intersection.entry[0]+':'+intersection.bearings[0], i;
                        for( i = 1; i < intersection.bearings.length; ++i )
                            string = string + ' ' + intersection.entry[i]+':'+intersection.bearings[i];
                        return string;
                    }).join(',');
            }).join(';');
    };

    this.modeList = (instructions) => {
        return this.extractInstructionList(instructions, s => s.mode);
    };

    this.timeList = (instructions) => {
        return this.extractInstructionList(instructions, s => s.duration + 's');
    };

    this.distanceList = (instructions) => {
        return this.extractInstructionList(instructions, s => s.distance + 'm');
    };

    this.weightName = (instructions) => {
        return instructions ? instructions.weight_name : '';
    };

    this.weightList = (instructions) => {
        return this.extractInstructionList(instructions, s => s.weight);
    };
};
