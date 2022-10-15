/* Gpx Viewer
 * Copyright (C) 2013-2015 Qball Cow <qball@sarine.nl>
 * Project homepage: http://blog.sarine.nl/

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

namespace Gpx
{
    public class FitFile : Gpx.FileBase
    {
        const int MAX_LOCAL_DEFINITIONS = 16;

        private Gpx.Track track = null;
        private uint32 data_length = 0;

        private enum FitTypes {
            LAP = 19,
            ACTIVITY_SUMMARY = 20
        }
        // Internal structs.
        private struct FieldDefinition{
            bool    endian; // false = little, true is big.
            uint16  type;
            FieldDefinitionHeader[] fields;
        }


        private FieldDefinition[] defs = new FieldDefinition[MAX_LOCAL_DEFINITIONS];
        private struct FieldDefinitionHeader
        {
            uint8 def_num;
            uint8 size;
            uint8 base_type;
        }

        /**
         * Get the field description belonging to the id.
         */
        private FieldDefinition *get_field_def(uint id) {
            if ( id >= MAX_LOCAL_DEFINITIONS ) error("To many local definitions specified.");
            return &defs[id];
        }

        /**
         * Depending on the definition, switch endianess
         */
        private void parse_apply_definition_endian(DataInputStream fs,FieldDefinition *def)
        {
            if(def->endian) {
                fs.set_byte_order(DataStreamByteOrder.BIG_ENDIAN);
            }else {
                fs.set_byte_order(DataStreamByteOrder.LITTLE_ENDIAN);
            }
        }

        /**
         * Parse the file.
         */
        public FitFile(File file) throws FileError
        {
            // Keep the pointer in the base class.
            this.file = file;
            try {
                var istr = file.read();
                // Open it and create an input stream.
                DataInputStream fs = new DataInputStream(istr);
                if(fs == null ) {
                    throw new FileError.IO_ERROR("Failed to create input stream.");
                }
                // Force the right endianess.
                if(fs.get_byte_order() != DataStreamByteOrder.LITTLE_ENDIAN) {
                    fs.set_byte_order(DataStreamByteOrder.LITTLE_ENDIAN);
                }

                //track = new Gpx.Track();
                // Parse the header file.
                this.parse_header(fs);
                // Parse all the records.
                while(this.parse_record(fs));


                // Add the track
                if(track != null) {
                    track.filter_points();
                    tracks.append(track);
                }
            } catch ( GLib.IOError err ) {
                throw new FileError.IO_ERROR(err.message);
            } catch ( GLib.Error err ) {
                throw new FileError.IO_ERROR(err.message);
            } catch ( FileError err) {
                throw err;
            }
        }

        /**
         * Parse record
         */
        private bool parse_record(DataInputStream fs)
        {
            try {
                if( data_length == 0 ) return false;
                uint8 record_id = fs.read_byte();
                data_length-=1;

                // Bit 7
                bool normal_header = (record_id & 0x80) == 0;

                bool definition_header;
                uint8 local_message_type;

                if(normal_header) {
                    // Bit 6
                    definition_header = (record_id & 0x40) > 0;
                    local_message_type = record_id&0x0F;

                } else {
                    // Compressed header
                    // Always data mesg.
                    definition_header = false;

                    local_message_type = (record_id & 0x60) >> 5;

                    // TODO: time offset
                }

                if( definition_header ) {
                    parse_definition_record(fs, local_message_type);
                }else {
                    parse_data_record(fs, local_message_type);
                }
            } catch (Error e ) {
                return false;
            }
            return true;
        }

        private void parse_definition_record(DataInputStream fs, uint8 local_message_type) throws FileError, GLib.IOError
        {
            FieldDefinition *def =  get_field_def(local_message_type);

            // Skip reserved byte.
            fs.skip(1);
            data_length-=1;

            // Check endianess
            uint8 endian = fs.read_byte();
            data_length-=1;
            if(endian == 1) {
                def.endian = true;
            } else {
                def.endian = false;
            }
            // Set the right endian
            parse_apply_definition_endian(fs,def);

            def.type = fs.read_uint16();
            data_length-=2;


            uint8 num_fields = fs.read_byte();
            data_length-=1;
            def.fields = new FieldDefinitionHeader[num_fields];
            for ( uint8 field =0; field < num_fields; field++) {
                FieldDefinitionHeader *header;
                uint8[] temp = new uint8[sizeof(FieldDefinitionHeader)];
                fs.read(temp);
                header = (FieldDefinitionHeader*)(&temp[0]);
                def.fields[field] = *header;
                data_length-=(uint32)sizeof(FieldDefinitionHeader);
            }
        }
        private uint32 parse_field(FieldDefinitionHeader field, DataInputStream fp) throws FileError, GLib.IOError
        {
            uint32 retv = 0;

            switch (field.base_type) {
                case 1: // sint8
                case 2: // uint8
                    retv = fp.read_byte();
                    data_length-=1;
                    break;
                case 0x83: // sint16
                case 0x84: // uint16
                    retv = fp.read_uint16();
                    data_length-=2;
                    break;
                case 0x85: // sint32
                case 0x86: // uint32
                    retv = fp.read_uint32();
                    data_length-=4;
                    break;
                default: // Ignore everything else for now.
                    fp.skip(field.size);
                    data_length-=field.size;
                    break;
            }
            return retv;
        }
        private void parse_data_record_lap(DataInputStream fs, FieldDefinition *def) throws FileError, GLib.IOError
        {

            foreach ( var field in def->fields ) {
                switch(field.def_num) {
                    case 11:
                        // Calories of the track?
                        uint32 val = parse_field(field, fs);
                        track.hrmt.calories = val;
                        break;
                    /** Currently we do not store other fields, as we do not care about them. */
                    case 15:
                        // AVG BPM
                        uint32 val = parse_field(field, fs);
                        break;
                    case 16:
                        // MAX BPM
                        uint32 val = parse_field(field, fs);
                        break;
                    default:
                        uint32 val = parse_field(field, fs);
                        debug("%d %u", field.def_num,val);
                        break;                        
                }
            }
            // Add the track
            if(track != null) {
                track.filter_points();
                tracks.append(track);
                track = null;
            }
        }
        private void parse_data_record_activity_summary(DataInputStream fs, FieldDefinition *def) throws FileError, GLib.IOError
        {
            if(track == null) {
                track = new Gpx.Track();
            }
            Gpx.Point  p = new Gpx.Point();
            foreach ( var field in def->fields ) {
                switch(field.def_num) {
                    case 253:
                        // Timestamp.
                        uint32 timestp = parse_field(field, fs);
                        timestp = timestp + 631065600;
                        Time t =  Time.local(timestp);
                        var str = t.format("%FT%T%z");
                        p.time = str;
                        break;
                    case 0:
                        // Longitude
                        uint32 val = parse_field(field, fs);
                        if(val != 0x7FFFFFFF) {
                            double lat_dec =  (double)val*(180.0/Math.pow(2.0,31.0));
                            p.set_position_lat(lat_dec);
                        }
                        break;
                    case 1:
                        // Longitude
                        uint32 val = parse_field(field, fs);
                        if(val != 0x7FFFFFFF) {
                            double lon_dec =  (double)val*(180.0/Math.pow(2.0,31.0));
                            p.set_position_lon(lon_dec);
                        }
                        break;
                    case 2:
                        // Elevation
                        uint32 val = parse_field(field, fs);
                        if ( val != 0xFFFF) {
                            p.elevation = val/5.0-500;
                        }
                        break;
                    case 3:
                        //  Heartrate
                        uint32 val = parse_field(field, fs);
                        if ( val != 0xFF ) {
                            p.tpe.heartrate = (int)val;
                        }
                        break;
                    case 4:
                        // Cadence.
                        uint32 val = parse_field(field, fs);
                        if ( val != 0xFF ) {
                            p.cadence = val;
                        }
                        break;
                    case 18:
                        // Cycles
                        uint32 val = parse_field(field, fs);
                        if ( val != 0xFF ) {
                            debug("Cycles: %u", val);
                        }
                        break;
                    default:
                        fs.skip(field.size);
                        data_length-=field.size;
                        break;
                }
            }
            // Fix up some points.
            // Ignore more tracks in one second.
            var lastp = track.get_last();
            if(lastp != null && lastp.get_time() == p.get_time()) {
                return;
            }
            if(p.has_position()) {
                    if(lastp != null && !lastp.has_position())
                    {
                        weak List<Gpx.Point> ll = track.points.last();
                        while(ll != null && !ll.data.has_position()){
                            var last = ll.data ;
                            last.lat_dec = p.lat_dec;
                            last.lon_dec = p.lon_dec;
                            last.lat= p.lat;
                            last.lon= p.lon;
                            ll = ll.prev;
                        }
                    }
                    track.add_point(p);
            }else {
                if(lastp != null) {
                    p.lat_dec = lastp.lat_dec;
                    p.lon_dec = lastp.lon_dec;
                    p.lat= lastp.lat;
                    p.lon= lastp.lon;
                    track.add_point(p);
                } else {
                    track.add_point(p);
                }
            }
        }
        private void parse_data_record(DataInputStream fs, uint8 local_message_type) throws FileError, GLib.IOError
        {
            FieldDefinition* def = get_field_def(local_message_type);

            // Set the right endian
            parse_apply_definition_endian(fs,def);
            switch(def.type)
            {
                case FitTypes.ACTIVITY_SUMMARY:
                    parse_data_record_activity_summary(fs, def);

                    break;
                case FitTypes.LAP:
                    parse_data_record_lap(fs, def);
                    break;
                default:
                    debug("Unknown record: %d", def.type);
                    foreach ( var field in def->fields ) {
                        fs.skip(field.size);
                        data_length-=field.size;
                    }
                    break;
            }

        }
        /**
         * Parse the header.
         */
        private void parse_header(DataInputStream fs) throws FileError, GLib.IOError
        {
            try {
                // Read header size. (though header is always 12?)
                // 1
                var header_size = (uchar)fs.read_byte();
                if ( header_size == FileStream.EOF ) {
                    throw new FileError.INVALID_FILE("Empty header");
                }
                if ( ! (header_size == 12 || header_size == 14) ) {
                    throw new FileError.INVALID_FILE("Unsupported FIT Header size");
                }

                // Get version
                // 2
                uchar version = (uchar)fs.read_byte();
                uchar low = version&0x0f;
                uchar high = (version&0xf0) >> 4;

                // 4
                uint16 profver = (uint16) fs.read_uint16();

                // 8
                data_length = (uint32)fs.read_uint32();

                // 12
                uint8[] signature = { 0,0,0,0};
                var size = fs.read(signature);
                if ( size == 4 ) {
                    if ( !(signature[0] == '.' && signature[1] == 'F' &&
                                signature[2] == 'I' && signature[3] == 'T'))
                    {
                        // Invalid signature.
                        throw new FileError.INVALID_FILE("No valid FIT signature found.");
                    }
                }
                // Size indicates there is a CRC.
                if(header_size == 14) {
                    uint16 crc = fs.read_uint16();
                }
            } catch (GLib.IOError err) {
                // propagate down.
                throw err;
            } catch (FileError err) {
               throw err;
            }
        }
    }
}
